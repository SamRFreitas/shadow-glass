# To do after Phase 1, step 1 — fix the author email in old commits

- Noted: 2026-10-07, in the `programmer` session working on step 1.
- Status: not started. Deliberately left until step 1 is finished, so
  the history is not rewritten in the middle of a test cycle.

## The situation

The correct identity is `SamRFreitas <samuelrfreitassi@gmail.com>`. It
was set in the Mac's global git config on 2026-10-07; commits from then
on are correct.

Earlier commits carry two wrong emails:

| Identity | Commits | Why it is wrong |
|---|---|---|
| `SamRFreitas <samuelfreitassi@gmail.com>` | 53 | typo: missing the "r" after "samuel" |
| `Samuel Ribeiro de Freitas <samrfreitas@Samuels-MacBook-Air.local>` | 20 | made up by git from the Mac's user and host name, when no identity was configured |

Counts as of 2026-10-07; check again with
`git log --format='%an <%ae>' | sort | uniq -c`.

Neither is a security problem: the email in a commit is a label, not a
credential. The effect is that GitHub does not link those commits to
Samuel's account.

## What fixing it means

Rewriting history: every commit is replaced by a new one with the same
content and the right author, so every commit hash changes.

- Needs a forced push to GitHub.
- The clone on the Acer (`C:\Users\samue\shadow-glass`) can no longer
  `git pull`; it needs `git fetch` and `git reset --hard origin/main`.
  Its `build` folder is kept.
- Commit hashes quoted in the notes and in commit messages (for
  example `02e0abb` in `note-first-cpp-code-2026-10-07.md`) stop
  pointing at anything and need updating.
- A `.mailmap` file does not help: GitHub ignores it when linking
  commits to profiles.

## How it should be done

As its own task, not mixed with other work: a backup copy of the
repository first, each command shown and explained before it runs, the
result checked before the forced push, and the person deciding at each
stage.
