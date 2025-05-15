import time
import json
import os
import asyncio
import argparse
from github import Github
from grpc import StatusCode
from nebius.sdk import SDK
from nebius.api.nebius.compute.v1 import (
    InstanceServiceClient,
    ListInstancesRequest,
    GetInstanceRequest,
)
from nebius.aio.service_error import RequestError
from .helpers import setup_logger, github_output

logger = setup_logger()


def filter_instances(instances, runners, args, now_ts):
    vms_to_remove = []
    matched_vm_ids = []
    idle_vm_ids = []
    busy_vm_ids = []

    for instance in instances:
        labels = instance.metadata.labels
        condition = (
            labels.get("repo", "") != args.github_repo
            or labels.get("owner", "") != args.github_repo_owner  # noqa: W503
            or labels.get("runner-flavor", "") != args.flavor  # noqa: W503
            or instance.status.state.name != "RUNNING"  # noqa: W503
        )
        logger.info(
            "Instance %s labels: %s, state: %s",
            instance.metadata.id,
            ", ".join([f"{k}: {v}" for k, v in labels.items()]),
            instance.status.state.name,
        )
        if condition:
            logger.info(
                "Instance %s does not match criteria: %s",
                instance.metadata.id,
                condition,
            )
            continue

        vm_id = instance.metadata.id
        matched_vm_ids.append(vm_id)
        age = int(now_ts - instance.metadata.created_at.timestamp())
        runner = next((r for r in runners if r.name == vm_id), None)
        logger.info(
            "Runner %s found: %s (id: %s, status: %s, busy: %s)",
            vm_id,
            runner,
            getattr(runner, "id", "N/A"),
            getattr(runner, "status", "N/A"),
            getattr(runner, "busy", "N/A"),
        )

        if runner and not runner.busy:
            idle_vm_ids.append(vm_id)
            if age > args.vms_older_than:
                logger.info(
                    "Instance %s is idle and its age is %d seconds (which is older than %d), marking for removal",
                    vm_id,
                    age,
                    args.vms_older_than,
                )
                vms_to_remove.append(vm_id)
        elif runner and runner.busy:
            logger.info("Instance %s is busy, not marking for removal", vm_id)
            busy_vm_ids.append(vm_id)

    return matched_vm_ids, idle_vm_ids, busy_vm_ids, vms_to_remove


# rules are:
# 1. If all alive VMs are busy and there are VMs to remove, raise an exception.
# 2. If there are no alive VMs, create the maximum number of VMs to create.
# 3. If there are idle VMs, check if the number of idle VMs exceeds the maximum number
# of VMs to create if so, remove the excess idle VMs.
# 4. If there are busy VMs and the number of alive VMs is less than the maximum number
# of VMs to have, create the extra VMs if needed.
# 5. If there are no idle VMs and the number of alive VMs is less than the maximum number
# of VMs to have, create extra vms if needed but not more than the maximum number of VMs
# to create.
def decide_scaling(
    alive: int,
    idle: int,
    busy: int,
    remove: int,
    max_vms_to_create: int,
    maximum_amount_of_vms_to_have: int,
    extra_vms_if_needed: int,
) -> tuple[int, int, int]:
    logger.info("alive=%d, idle=%d, busy=%d, remove=%d", alive, idle, busy, remove)
    logger.info(
        "max_vms_to_create=%d, maximum_amount_of_vms_to_have=%d, extra_vms_if_needed=%d",
        max_vms_to_create,
        maximum_amount_of_vms_to_have,
        extra_vms_if_needed,
    )
    if alive == busy and remove > 0:
        raise ValueError("Cannot remove VMs when all alive VMs are busy. ")
    if idle + busy != alive:
        raise ValueError(
            "Passed values are not valid idle=%d + busy=%d != alive=%d"
            % (alive, idle, busy)
        )

    if alive == 0:
        logger.info(
            "No alive VMs, creating %d VM(s) to meet minimum target", max_vms_to_create
        )
        return max_vms_to_create, 0, max_vms_to_create

    excess_idle = max(0, idle - max_vms_to_create)
    idle_remaining = idle - excess_idle
    projected_preview = alive - remove

    to_create = 0
    logger.info(
        "excess_idle=%d, projected_preview=%d, idle_remaining=%d, busy=%d",
        excess_idle,
        projected_preview,
        idle_remaining,
        busy,
    )
    if projected_preview == 0:
        to_create = max_vms_to_create
        logger.info(
            "No VMs projected to remain, creating %d VM(s) to meet minimum target",
            to_create,
        )
    elif idle_remaining + busy < max_vms_to_create:
        to_create = max_vms_to_create - (idle_remaining + busy)
        logger.info(
            "Not enough total VMs available (idle + busy = %d), creating %d VM(s) to reach the minimum required",
            idle_remaining + busy,
            to_create,
        )
    elif (
        busy >= max(1, projected_preview - 1)
        and projected_preview < maximum_amount_of_vms_to_have  # noqa: W503
    ):
        to_create = min(
            extra_vms_if_needed, maximum_amount_of_vms_to_have - projected_preview
        )
        logger.info("Most VMs are busy, provisioning %d extra VM(s)", to_create)

    projected = projected_preview + to_create - excess_idle
    if projected > maximum_amount_of_vms_to_have:
        raise ValueError(
            "Projected VMs (%s) exceed the maximum amount of VMs to have (%s).",
            projected,
            maximum_amount_of_vms_to_have,
        )

    return to_create, excess_idle, projected


async def run(github: Github, sdk: SDK, args: argparse.Namespace):
    now_ts = int(time.time())
    repo = github.get_repo(f"{args.github_repo_owner}/{args.github_repo}")
    instance_client = InstanceServiceClient(sdk)
    instances = []
    try:
        logger.info("Listing instances from Nebius (with pagination)...")
        request = ListInstancesRequest(parent_id=args.parent_id)
        while True:
            response = await instance_client.list(request)
            instances.extend(response.items)
            if not response.next_page_token:
                break
            request.page_token = response.next_page_token
    except RequestError as err:
        logger.error("Failed to fetch instances from Nebius: %s", err)
        github_output("RUNNING_VMS_COUNT", "0")
        github_output("VMS_TO_REMOVE", "[]")
        github_output("VMS_TO_CREATE", "[]")
        github_output("DATE", str(now_ts))
        return

    logger.info("Fetched %d instances", len(instances))
    runners = list(repo.get_self_hosted_runners())

    matched_vm_ids, idle_vm_ids, busy_vm_ids, vms_to_remove = filter_instances(
        instances, runners, args, now_ts
    )

    logger.info(
        "Total matched VMs: %d (Idle: %d, Busy: %d)",
        len(matched_vm_ids),
        len(idle_vm_ids),
        len(busy_vm_ids),
    )

    to_create, excess_idle, projected_vm_count = decide_scaling(
        len(matched_vm_ids),
        len(idle_vm_ids),
        len(busy_vm_ids),
        len(vms_to_remove),
        args.max_vms_to_create,
        args.maximum_amount_of_vms_to_have,
        args.extra_vms_if_needed,
    )

    if excess_idle > 0:
        to_remove = idle_vm_ids[:excess_idle]
        logger.info(
            "Excess idle VMs: %d, marking %d for removal",
            excess_idle,
            len(to_remove),
        )
        to_remove.extend([vm_id for vm_id in to_remove if vm_id not in vms_to_remove])

    logger.info("PROJECTED_VM_COUNT=%d", projected_vm_count)
    logger.info("FINAL_TO_CREATE=%d", to_create)
    logger.info("FINAL_TO_REMOVE=%d", len(vms_to_remove))

    vms_to_create = (
        [
            f"{args.flavor}-{args.github_repo_owner}-{args.github_repo}-{now_ts}-{i+1}"
            for i in range(to_create)
        ]
        if to_create > 0
        else []
    )

    github_output("VMS_TO_REMOVE", json.dumps(vms_to_remove))
    github_output("VMS_TO_CREATE", json.dumps(vms_to_create))

    # clean up github runners that doesn't have a matching VM and are offline
    logger.info("Cleaning up GitHub runners that don't have a matching VM")
    for runner in list(repo.get_self_hosted_runners()):
        logger.info(
            "Runner %s (id: %s, status: %s, busy: %s)",
            runner.name,
            runner.id,
            runner.status,
            runner.busy,
        )
        if runner.status != "offline":
            logger.info("Runner %s is not offline, skipping", runner.name)
            continue

        try:
            request = GetInstanceRequest(id=runner.name)
            instance = await instance_client.get(request)
            if instance.status.state.name == "RUNNING":
                logger.info(
                    "Runner %s has a matching VM, skipping removal", runner.name
                )
                continue
        except RequestError as err:
            if err.status.code == StatusCode.NOT_FOUND:
                logger.info(
                    "Runner %s does not have a matching VM, removing runner",
                    runner.name,
                )
                if not repo.remove_self_hosted_runner(runner):
                    logger.error(
                        "Failed to remove runner %s (id: %s)", runner.name, runner.id
                    )
                    return
                logger.info("Removed runner %s (id: %s)", runner.name, runner.id)


async def main():
    parser = argparse.ArgumentParser(description="Manage GitHub runners on Nebius.")
    parser.add_argument(
        "--api-endpoint", default="api.ai.nebius.cloud", help="Cloud API endpoint"
    )
    parser.add_argument(
        "--service-account-key",
        required=True,
        help="Path to the service account credentials file (JSON)",
    )
    parser.add_argument(
        "--github-repo-owner",
        required=True,
        default="ydb-platform",
        help="GitHub repository owner name",
    )
    parser.add_argument(
        "--github-repo", required=True, default="nbs", help="GitHub repository name"
    )
    parser.add_argument(
        "--parent-id",
        required=True,
        help="Parent folder or project ID for VM placement",
    )
    parser.add_argument(
        "--flavor",
        required=True,
        choices=["light", "heavy"],
        help="VM flavor label to match against",
    )
    parser.add_argument(
        "--vms-older-than",
        type=int,
        required=True,
        help="Minimum VM age (in seconds) before it can be deleted if idle",
    )
    parser.add_argument(
        "--max-vms-to-create",
        type=int,
        required=True,
        help="Maximum number of VMs to create if idle VMs are less than this",
    )
    parser.add_argument(
        "--maximum-amount-of-vms-to-have",
        type=int,
        required=True,
        help="Hard cap on total number of VMs allowed",
    )
    parser.add_argument(
        "--extra-vms-if-needed",
        type=int,
        default=1,
        help="Number of additional VMs to create when most are busy",
    )
    args = parser.parse_args()
    logger.info("Parsed arguments: %s", args)

    github_token = os.environ.get("GITHUB_TOKEN")
    if not github_token:
        raise RuntimeError("GITHUB_TOKEN environment variable is not set")

    sdk = SDK(credentials_file_name=args.service_account_key)
    github = Github(github_token)

    async with sdk:
        await run(github, sdk, args)


if __name__ == "__main__":
    asyncio.run(main())
