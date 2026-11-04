"""Kiadás kizárólag az adott main commitba összevont PR után, egyjegyű verziókkal."""

import json
import os
import re
import urllib.request


def next_version(tags):
    versions = [tuple(map(int, tag[1:].split("."))) for tag in tags if re.fullmatch(r"v\d\.\d\.\d", tag)]
    if not versions:
        return "0.1.0"
    major, minor, patch = max(versions)
    patch += 1
    if patch == 10:
        minor, patch = minor + 1, 0
    if minor == 10:
        major, minor = major + 1, 0
    if major == 10:
        raise ValueError("Az egyjegyű verziótartomány elfogyott")
    return f"{major}.{minor}.{patch}"


def merged_pr(pulls, sha):
    return next((pull for pull in pulls if pull.get("merged_at") and pull.get("merge_commit_sha") == sha
                 and pull.get("base", {}).get("ref") == "main"), None)


def github(endpoint):
    request = urllib.request.Request("https://api.github.com/" + endpoint, headers={
        "Accept": "application/vnd.github+json", "Authorization": "Bearer " + os.environ["GH_TOKEN"],
        "X-GitHub-Api-Version": "2022-11-28", "User-Agent": "ZoleeSchemaMonitor-CI"})
    with urllib.request.urlopen(request, timeout=30) as response:
        return json.load(response)


if __name__ == "__main__":
    sha, repository = os.environ["GITHUB_SHA"], os.environ["GITHUB_REPOSITORY"]
    publish, version, pr = False, "0.1.0-ms1." + sha[:7], None
    if os.environ["GITHUB_EVENT_NAME"] == "push" and os.environ["GITHUB_REF"] == "refs/heads/main":
        pr = merged_pr(github(f"repos/{repository}/commits/{sha}/pulls"), sha)
        if pr:
            tags = []
            page = 1
            while True:
                response = github(f"repos/{repository}/tags?per_page=100&page={page}")
                tags += [tag["name"] for tag in response]
                if len(response) < 100:
                    break
                page += 1
            version, publish = next_version(tags), True
    with open(os.environ["GITHUB_OUTPUT"], "a") as output:
        output.write(f"version={version}\npublish={'true' if publish else 'false'}\npr={pr['number'] if pr else ''}\n")
