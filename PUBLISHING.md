# 📦 HPR-Store Publishing Guide

This document explains how to submit your extension or theme to HPR-Store so that all users can browse and install it from within HPR.

---

## Submission Procedure

1. **Create a GitHub Release** with a `.zip` file containing your extension or theme folder.
2. **Fork this repository**, add your entry to `registry.json`, and open a Pull Request.
3. **Once merged**, all HPR-Store users will see your extension on their next database refresh.

> [!IMPORTANT]
> **Review times:** I personally read every submission — I'm the sole developer maintaining HPR-Store. For open-source extensions, I review the Lua and C++ source code directly. For closed-source extensions, review may take significantly longer — I may need to observe runtime behaviour or decompile the binary before approving. Please be patient.

---

## Submission Rules

### Rule 1 — ID Prefix

The `hpr-` prefix in the `id` field is **reserved exclusively for official HPR extensions** developed by the HPR team. Third-party submissions **must not** use the `hpr-` prefix.

✅ `"id": "my-cool-extension"`  
❌ `"id": "hpr-my-cool-extension"`

---

### Rule 2 — Native Tag Required for Native Extensions

If your extension loads a `.dll` or `.so` native library at runtime, you **must** include the `"native"` tag in your `tags` array. HPR warns users before they enable native extensions. Omitting this tag is grounds for immediate rejection.

```json
"tags": ["native", "your-other-tags"]
```

---

### Rule 3 — No Malicious Code

Submissions must not contain viruses, spyware, ransomware, cryptocurrency miners, keyloggers, backdoors, or any other malicious or deceptive code. Any submission found to violate this rule will be rejected and the author will be **permanently banned** from the registry.

---

### Rule 4 — Open Source Policy

Open source is **not required** — it is your choice. However, closed-source extensions with vague or unclear descriptions of their behaviour face a **significantly higher chance of rejection**, as I cannot verify what the extension does without significant additional effort. If your code is public, include `sourceUrl` in your entry — it greatly speeds up my review.

---

### Rule 5 — Working Download URL

`downloadUrl` must be a **direct, publicly accessible** link to a `.zip` archive containing your extension folder. GitHub Release asset links are the recommended format. Dead links, gated URLs, or links that require login will be rejected.

---

### Rule 6 — Accurate Metadata

Your `name`, `description`, `version`, and `type` fields must **accurately and honestly** describe what your extension does. Misleading, exaggerated, or deceptive metadata is grounds for rejection.

---

### Rule 7 — Versioning

Use a consistent version string (e.g. `"1.0"`, `"0.3.1"`). **Never reuse the same version string for a different build.** When you release an update, bump the version and submit a new PR updating `version`, `downloadUrl`, and `lastUpdated`.

---

### Rule 8 — No Impersonation

Do not impersonate other authors, projects, or HPR branding in any metadata field (`name`, `description`, `author`, `id`, etc.). This includes names or identifiers that are confusingly similar to official HPR extensions.

---

### Rule 9 — No Duplicate Listings for the Same Extension

You may not submit duplicate entries for the exact same codebase or extension project (e.g. listing your same theme twice under different IDs or names). 

> [!NOTE]
> This does **not** prevent different developers from publishing competing extensions that do the same thing (for example, another author is fully allowed to publish their own version of an "idle detection" extension). It only prevents duplicate spam listings of the same project.

---

### Rule 10 — Publishing Cooldown

A single author may not publish **more than one new extension within any 10-day rolling window**. I am the sole maintainer and must personally review every submission — this limit keeps the queue manageable. PRs that violate this cooldown will be held until the window has passed.

> [!NOTE]
> The cooldown applies only to **new extension submissions**. Updating an existing entry (bumping version, fixing a download URL, etc.) has **no cooldown**.

---

### Rule 11 — Maintainability

You are responsible for keeping your entry up to date — particularly `downloadUrl` and `version`. Entries with broken download links or that are demonstrably abandoned may be removed from the registry after the author has been notified.

---

## `registry.json` Entry Schema

Add your entry to the `registry.json` array. All fields are described below.

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `id` | string | ✅ | Unique kebab-case identifier (e.g. `"my-extension"`). Must **not** use the `hpr-` prefix. |
| `name` | string | ✅ | Display name shown in the store UI. |
| `author` | string | ✅ | Your name or GitHub username. |
| `authorGithub` | string | — | Full URL to your GitHub profile. |
| `description` | string | ✅ | Short one-line description shown in the store listing. |
| `longDescription` | string | — | Full Markdown description shown in the extension detail view. |
| `version` | string | ✅ | Current version string (e.g. `"0.2"`, `"1.0.3"`). |
| `downloadUrl` | string | ✅ | Direct public URL to the release `.zip` archive. |
| `sourceUrl` | string | — | URL to the public source code repository. Strongly recommended, especially for closed-source extensions. |
| `type` | string | ✅ | `"EXTENSION"` or `"THEME"`. |
| `tags` | array | — | Searchable tag strings. Must include `"native"` if your extension loads a native library (see Rule 2). |
| `supportedHPRVersions` | array | — | List of compatible HPR version strings (e.g. `["0.9.7"]`). |
| `previewImages` | array | — | URLs to preview images displayed in the UI carousel. |
| `downloadCount` | int | — | Download counter — maintained by the registry, set to `0` on submission. |
| `starCount` | int | — | Star counter — maintained by the registry, set to `0` on submission. |
| `lastUpdated` | string | — | ISO date of last update (e.g. `"2026-08-16"`). |

### Example Entry

```json
{
  "id": "my-extension",
  "name": "My Extension",
  "author": "yourname",
  "authorGithub": "https://github.com/yourname",
  "description": "A short description of what this extension does.",
  "longDescription": "### My Extension\n\nLonger Markdown description here.",
  "version": "1.0",
  "downloadUrl": "https://github.com/yourname/my-extension/releases/download/v1.0/my-extension.zip",
  "sourceUrl": "https://github.com/yourname/my-extension",
  "type": "EXTENSION",
  "tags": ["your", "tags"],
  "supportedHPRVersions": ["0.9.7"],
  "previewImages": [],
  "downloadCount": 0,
  "starCount": 0,
  "lastUpdated": "2026-08-16"
}
```

---

## After Your PR is Merged

Once your PR is merged into `main`:

- HPR-Store users will see your extension the next time they click **Refresh** in the store UI (which re-downloads `registry.json` from GitHub).
- No action is needed on your part — the store picks up new entries automatically.

### Updating Your Entry Later

To release a new version, submit another PR that updates your existing entry with:
- `version` — bumped to the new version string
- `downloadUrl` — pointing to the new release archive
- `lastUpdated` — updated to today's date

There is **no cooldown** on update PRs.
