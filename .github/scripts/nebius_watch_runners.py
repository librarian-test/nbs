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
        for job in run.get_jobs():
            if job.status in ("in_progress", "queued") and job.runner_name:
                active_jobs[job.runner_name] = {
                    "job_name": job.name,
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
        labels = [label["name"] for label in runner.labels]
        runner_label = next((l for l in labels if l.startswith("runner_")), "")
        current_job = active_jobs.get(name)

        job_info = (
            f'{current_job["job_name"]} ({current_job["workflow"]})'
            if current_job
            else ""
        )

        table.append([runner_id, name, status, busy, runner_label, job_info])

    # Display
    headers = ["ID", "Runner Name", "Status", "Busy", "Runner Label", "Current Job"]
    print(tabulate(table, headers=headers, tablefmt="github"))


if __name__ == "__main__":
    main()
