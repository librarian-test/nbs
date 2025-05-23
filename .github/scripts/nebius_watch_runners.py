#!/usr/bin/env python3
import os
import asyncio
import argparse
from github import Github
from tabulate import tabulate
from .helpers import setup_logger
import datetime

from nebius.sdk import SDK
from nebius.aio.service_error import RequestError
from nebius.api.nebius.compute.v1 import (
    InstanceServiceClient,
    GetInstanceRequest,
)

logger = setup_logger()


def parse_args():
    parser = argparse.ArgumentParser(
        description="Show self-hosted runners and their active jobs."
    )
    parser.add_argument(
        "--api-endpoint",
        default="api.ai.nebius.cloud",
        help="Cloud API Endpoint",
    )
    parser.add_argument(
        "--service-account-key",
        required=True,
        help="Path to the service account key file",
    )
    parser.add_argument("--owner", required=True, help="GitHub organization or user")
    parser.add_argument("--repo", required=True, help="GitHub repository name")
    parser.add_argument(
        "--token", help="GitHub access token (or set GITHUB_TOKEN env variable)"
    )
    return parser.parse_args()


def created_at_to_formatted_string(created_at: datetime.datetime) -> str:
    """Convert a datetime object to a formatted string."""
    now = datetime.datetime.now(datetime.timezone.utc)
    age = now - created_at

    age_days = age.days
    age_hours, remainder = divmod(age.seconds, 3600)
    age_minutes, _ = divmod(remainder, 60)

    if age_days > 0:
        return f"{age_days}d{age_hours}h{age_minutes}m"
    elif age_hours > 0:
        return f"{age_hours}h{age_minutes}m"
    else:
        return f"{age_minutes}m"


def compact_job_name(job_name: str) -> str:
    """Convert a job name to a compact format."""
    if job_name.startswith("Build and test"):
        return job_name.replace("Build and test", "Build").strip()
    if job_name.startswith("Populate VMs"):
        return job_name.split("(")[0].strip()
    return job_name


async def main():
    args = parse_args()
    token = args.token or os.environ.get("GITHUB_TOKEN")
    if not token:
        print(
            "Error: GitHub token must be provided with --token or GITHUB_TOKEN environment variable."
        )
        exit(1)

    sdk = SDK(credentials_file_name=args.service_account_key)
    service = InstanceServiceClient(sdk)

    g = Github(token)
    repo = g.get_repo(f"{args.owner}/{args.repo}")

    # Fetch self-hosted runners
    runners = repo.get_self_hosted_runners()

    # Map of runner name -> current job
    active_jobs = {}
    workflow_runs = repo.get_workflow_runs(status="in_progress")

    for run in workflow_runs:
        for job in run.jobs():
            if job.status in ("in_progress", "queued") and job.runner_name:
                active_jobs[job.runner_name] = {
                    "job_name": job.name,
                    "job_id": job.id,
                    "run_id": run.id,
                    "workflow": run.name,
                    "html_url": job.html_url,
                }

    # Prepare data
    table = []
    for runner in runners:
        runner_id = runner.id
        name = runner.name
        status = runner.status
        busy = runner.busy
        current_job = active_jobs.get(name)
        runner_label = ", ".join(
            label["name"]
            for label in runner.labels()
            if label["name"].startswith("runner")
        )
        job_info = (
            f'{current_job["job_name"].split("/")[-1].strip()}' if current_job else ""
        )

        workflow_info = f'{current_job["workflow"]}' if current_job else ""

        job_id = f'{current_job["job_id"]}' if current_job else ""
        workflow_id = f'{current_job["run_id"]}' if current_job else ""

        # calculate age of the instance
        try:
            response = await service.get(GetInstanceRequest(id=name))
        except RequestError:
            logger.error(f"Error fetching instance {runner_id}")

        age_str = created_at_to_formatted_string(response.metadata.created_at)

        table.append(
            [
                runner_id,
                age_str,
                name,
                status,
                "BUSY" if busy else "FREE",
                runner_label.replace("runner_", "").strip(),
                workflow_info,
                compact_job_name(job_info),
                workflow_id,
                job_id,
            ]
        )

    # Display
    headers = [
        "ID",
        "Age",
        "Runner Name",
        "Status",
        "Busy",
        "Type",
        "Workflow",
        "Job",
        "Workflow ID",
        "Job ID",
    ]
    print(tabulate(table, headers=headers))


if __name__ == "__main__":
    asyncio.run(main())
