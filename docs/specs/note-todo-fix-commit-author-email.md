# To do after Phase 1, step 1 — fix the author email in old commits

- Noted: 2026-10-07, in the `programmer` session working on step 1.
- Status: **done on 2026-10-07**, earlier than planned: the person
  asked for it at the end of the day, with the tree clean, everything
  pushed and no test running on the Acer. See "What was done" at the
  end. The sections before it are kept as written beforehand.

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

## What was done (2026-10-07)

1. Backup: `git clone --mirror` of the repository to
   `~/Lab/shadow-glass-backup-2026-10-07.git`, checked against the
   original (77 commits, same head, no integrity errors).
2. Rewrite, on the Mac only: `git filter-branch --env-filter` on
   `main`, replacing author and committer wherever the email was one of
   the two wrong ones. `git filter-repo` was not installed; for 77
   commits the built-in tool was enough.
3. Check before pushing: 77 commits before and after; the final file
   tree identical; every commit's tree, author date, committer date and
   full message identical, compared one by one; 77 of 77 with
   `SamRFreitas <samuelrfreitassi@gmail.com>` as author and committer.
4. `git push --force-with-lease origin main`, run by the person:
   `4e7de41...7d29bdb main -> main (forced update)`. A fetch afterwards
   confirmed GitHub and the Mac at the same commit.

Hashes quoted in the notes were updated: `02e0abb` became `79001aa`,
`c1611a1` became `6986e21`.

Still to do:

- On the Acer, before its next use: `git fetch`, then
  `git reset --hard origin/main` (a plain `git pull` no longer works
  there). Its `build` folder is kept.
- Delete the backup folder once nothing has gone wrong for a few days.

This corrects how the commits are attributed from now on. It does not
erase the old emails from the internet: GitHub keeps unreachable
commits for some time, and any earlier copy of the repository has them.
