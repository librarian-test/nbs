#!/usr/bin/env python3

import os
import argparse
import datetime
import requests
import numpy as np
from github import Github
from tabulate import tabulate
from collections import defaultdict
from dateutil import parser as dateparser
from dateutil.relativedelta import relativedelta
from .helpers import setup_logger
from typing import List

from dataclasses import dataclass

logger = setup_logger()


@dataclass
class Job:
    workflow: str
    id: int
    run_id: int
    name: str
    runner: str
    queued_at: datetime.datetime
    started_at: datetime.datetime
    wait_sec: float

    def __post_init__(self):
        if self.wait_sec is None:
            self.wait_sec = 0.0
        else:
            self.wait_sec = (self.started_at - self.queued_at).total_seconds()


# --- Utilities ---
def parse_datetime(value, now=None):
    now = now or datetime.datetime.now(datetime.timezone.utc)
    try:
        if value.endswith("d"):
            days = int(value.rstrip("d"))
            return now - relativedelta(days=days)
        elif value == "now":
            return now
        else:
            return dateparser.isoparse(value)
    except ValueError:
        raise argparse.ArgumentTypeError(
            f"Invalid datetime or relative format: {value}"
        )


def classify_runner(labels):
    if "self-hosted" in labels:
        if "runner_light" in labels:
            return "runner_light"
        elif "runner_heavy" in labels:
            return "runner_heavy"
        else:
            return "runner_none"
    else:
        image_labels = [label for label in labels if label not in ("Linux", "X64")]
        return f"{image_labels[0]}" if image_labels else "unknown"


def get_jobs_raw(repo_full_name, run_id):
    url = f"https://api.github.com/repos/{repo_full_name}/actions/runs/{run_id}/jobs?per_page=100"
    headers = {
        "Authorization": f"Bearer {GITHUB_TOKEN}",
        "Accept": "application/vnd.github+json",
    }
    response = requests.get(url, headers=headers)
    response.raise_for_status()
    return response.json()["jobs"]


def output_results(all_jobs: List[Job], summary):
    print("=== Job Wait Times ===")
    print(
        tabulate(
            [
                [
                    f"{job.run_id}:{job.id}",
                    job.workflow.replace(".yaml", "").replace(".yml", ""),
                    job.name,
                    job.runner,
                    job.queued_at.isoformat() if job.queued_at else "N/A",
                    job.started_at.isoformat() if job.started_at else "N/A",
                    job.wait_sec,
                ]
                for job in all_jobs
            ],
            headers=[
                "Id",
                "Workflow",
                "Job Name",
                "Runner Type",
                "Queued At",
                "Started At",
                "Wait (s)",
            ],
        )
    )

    print("=== Summary ===")
    print(
        tabulate(
            [
                [
                    runner,
                    data["count"],
                    int(data["total_wait"]),
                    (
                        round(data["total_wait"] / data["count"], 2)
                        if data["count"]
                        else 0
                    ),
                    int(np.percentile(data["waits"], 1)) if data["waits"] else "N/A",
                    int(np.percentile(data["waits"], 50)) if data["waits"] else "N/A",
                    int(np.percentile(data["waits"], 99)) if data["waits"] else "N/A",
                ]
                for runner, data in summary.items()
            ],
            headers=[
                "Runner Type",
                "Job Count",
                "Total Wait (s)",
                "Avg Wait (s)",
                "P1 (s)",
                "Median (s)",
                "P99 (s)",
            ],
        )
    )


def main(start, end):
    logger.info(f"Fetching workflow runs from {start} to {end}")
    all_jobs = []
    summary = defaultdict(lambda: {"total_wait": 0.0, "count": 0, "waits": []})

    runs = repo.get_workflow_runs()
    for run in runs:
        created_at = run.created_at
        if created_at < start:
            logger.info("Reached runs before target window; stopping.")
            break
        if created_at >= end:
            continue

        try:
            jobs = get_jobs_raw(repo.full_name, run.id)
        except Exception as e:
            logger.warning(f"Failed to get jobs for run {run.id}: {e}")
            continue

        for job in jobs:
            name = job["name"]
            queued_at = job.get("created_at")
            started_at = job.get("started_at")
            conclusion = job.get("conclusion")
            labels = job.get("labels", [])

            if conclusion == "skipped":
                logger.debug(f"Job {name} was skipped; skipping.")
                continue

            if conclusion == "cancelled" and len(labels) == 0:
                logger.debug(f"Job {name} was cancelled; skipping.")
                continue

            wait_sec = None
            if queued_at and started_at:
                queued_dt = datetime.datetime.fromisoformat(
                    queued_at.replace("Z", "+00:00")
                )
                started_dt = datetime.datetime.fromisoformat(
                    started_at.replace("Z", "+00:00")
                )
                wait_sec = (started_dt - queued_dt).total_seconds()

            runner_type = classify_runner(labels)
            name_string = name.split("/")[-1].strip()
            # remove anything inside [] brackets
            name_string = name_string.split("[")[0].strip()
            all_jobs.append(
                Job(
                    workflow=run.path.split("/")[-1],
                    id=job["id"],
                    run_id=run.id,
                    name=name_string,
                    runner=runner_type,
                    queued_at=queued_dt,
                    started_at=started_dt,
                    wait_sec=wait_sec,
                )
            )

            if wait_sec is not None:
                summary[runner_type]["total_wait"] += wait_sec
                summary[runner_type]["count"] += 1
                summary[runner_type]["waits"].append(wait_sec)

    output_results(all_jobs, summary)


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Analyze GitHub Actions job wait times."
    )
    parser.add_argument(
        "--since",
        type=str,
        default="1d",
        help="Start of time window (e.g. 2d, 2025-05-20T00:00:00Z)",
    )
    parser.add_argument(
        "--until",
        type=str,
        default="now",
        help="End of time window (e.g. now, 2025-05-21T00:00:00Z)",
    )
    parser.add_argument(
        "--owner",
        type=str,
        default="librarian-test",
        help="GitHub repository owner ",
    )
    parser.add_argument(
        "--repo",
        type=str,
        default="nbs",
        help="GitHub repository name ",
    )
    parser.add_argument(
        "--token",
        type=str,
        help="GitHub token for authentication",
    )
    args = parser.parse_args()

    now = datetime.datetime.now(datetime.timezone.utc)
    start = parse_datetime(args.since, now)
    end = parse_datetime(args.until, now)

    GITHUB_TOKEN = args.token if args.token else os.getenv("GITHUB_TOKEN")
    if not GITHUB_TOKEN:
        raise EnvironmentError(
            "GITHUB_TOKEN environment variable not set or passed as argument."
        )

    g = Github(GITHUB_TOKEN)
    repo = g.get_repo(f"{args.owner}/{args.repo}")

    main(start, end)
