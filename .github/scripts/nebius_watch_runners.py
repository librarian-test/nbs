#!/usr/bin/env python3
import os
import argparse
from github import Github
from tabulate import tabulate


def parse_args():
    parser = argparse.ArgumentParser(
        description="Show self-hosted runners and their active jobs."
    )
    parser.add_argument("--owner", required=True, help="GitHub organization or user")
    parser.add_argument("--repo", required=True, help="GitHub repository name")
    parser.add_argument(
        "--token", help="GitHub access token (or set GITHUB_TOKEN env variable)"
    )
    return parser.parse_args()


def main():
    args = parse_args()
    token = args.token or os.environ.get("GITHUB_TOKEN")
    if not token:
        print(
            "Error: GitHub token must be provided with --token or GITHUB_TOKEN environment variable."
        )
        exit(1)

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

        table.append(
            [
                runner_id,
                name,
                status,
                busy,
                runner_label,
                job_info,
                job_id,
                workflow_info,
                workflow_id,
            ]
        )

    # Display
    headers = [
        "ID",
        "Runner Name",
        "Status",
        "Busy",
        "Runner Label",
        "Job",
        "Job ID",
        "Workflow",
        "Workflow ID",
    ]
    print(tabulate(table, headers=headers))


if __name__ == "__main__":
    main()
