#!/usr/bin/env python3
"""Print GitHub release asset download counts as CSV or JSON.

Example:
    python3 scripts/github_download_metrics.py
    GITHUB_TOKEN=... python3 scripts/github_download_metrics.py --format json
"""

import argparse
import csv
import json
import os
import sys
from urllib.error import HTTPError, URLError
from urllib.request import Request, urlopen


API_URL = "https://api.github.com"
FIELDS = ("release", "tag", "published_at", "asset", "download_count")


def get_releases(owner, repo, token=None):
    """Return all releases, following GitHub's paginated releases endpoint."""
    releases = []
    page = 1

    while True:
        url = f"{API_URL}/repos/{owner}/{repo}/releases?per_page=100&page={page}"
        headers = {
            "Accept": "application/vnd.github+json",
            "User-Agent": "fceux-download-metrics",
        }
        if token:
            headers["Authorization"] = "Bearer " + token

        request = Request(url, headers=headers)
        with urlopen(request, timeout=30) as response:
            batch = json.load(response)

        if not isinstance(batch, list):
            raise ValueError("GitHub API returned an unexpected releases response")
        releases.extend(batch)
        if len(batch) < 100:
            return releases
        page += 1


def asset_rows(releases):
    for release in releases:
        for asset in release.get("assets", []):
            yield {
                "release": release.get("name") or release.get("tag_name", ""),
                "tag": release.get("tag_name", ""),
                "published_at": release.get("published_at") or "",
                "asset": asset.get("name", ""),
                "download_count": asset.get("download_count", 0),
            }


def main():
    parser = argparse.ArgumentParser(
        description="Fetch download counts for GitHub release assets."
    )
    parser.add_argument("--owner", default="TASEmulators")
    parser.add_argument("--repo", default="fceux")
    parser.add_argument(
        "--format", choices=("csv", "json"), default="csv",
        help="output format (default: csv)",
    )
    args = parser.parse_args()

    try:
        rows = list(asset_rows(get_releases(
            args.owner, args.repo, os.environ.get("GITHUB_TOKEN")
        )))
    except (HTTPError, URLError, TimeoutError, ValueError) as error:
        print(f"error: unable to fetch GitHub release metrics: {error}", file=sys.stderr)
        return 1

    if args.format == "json":
        json.dump(rows, sys.stdout, indent=2)
        print()
    else:
        writer = csv.DictWriter(sys.stdout, fieldnames=FIELDS)
        writer.writeheader()
        writer.writerows(rows)
    return 0


if __name__ == "__main__":
    sys.exit(main())
