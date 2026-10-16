# AGENTS.md

Supplementary repository for a paper. It pins the SSHash-family tools and their masked-superstring (`bigreedy-ms`) modifications as git submodules. All development and syncing happens **through this repository**, not in separate standalone clones.

## Layout

| Dir | Repo | Role | Branch | Upstream |
|---|---|---|---|---|
| `01_sshash-lite` | jermp/sshash-lite | original | `main` | – |
| `02_sshash` | jermp/sshash | original | `master` | – |
| `03_sshash-rs` | COMBINE-lab/sshash-rs | original | `main` | – |
| `11_sshash-lite-bigreedy-ms` | karel-brinda/sshash-lite-bigreedy-ms | modified | `main` | `01` (jermp/sshash-lite `main`) |
| `12_sshash-bigreedy-ms` | karel-brinda/sshash-bigreedy-ms | modified | `master` | `02` (jermp/sshash `master`) |
| `13_sshash-rs-bigreedy-ms` | karel-brinda/sshash-rs-bigreedy-ms | modified | `main` | `03` (COMBINE-lab/sshash-rs `main`) |

Numbering: `0x` = original, `1x` = modification of `0x`. The pinned `0x` commit should be the fork point (merge base) of the corresponding `1x`.

## Rules

- Submodules are pinned to **exact commits**. `.gitmodules` has no `branch =` entries, so do not rely on `git submodule update --remote`.
- Never modify `01`–`03`. They only move when deliberately bumped to a new upstream commit.
- Make code changes only inside `11`–`13`: commit and push in the submodule first, then commit the updated pointer in this repo. Never commit a pointer to a commit that hasn't been pushed.
- `.gitmodules` uses public HTTPS URLs (for readers of the paper). For pushing, set a local SSH push URL in each `1x` (not committed):
  `git -C 1x_... remote set-url --push origin git@github.com:karel-brinda/<repo>.git`
- Each `1x` has a local `upstream` remote pointing to its original repo, plus `git config checkout.defaultRemote origin` (both remotes have the same branch names, so `git switch main` is otherwise ambiguous).
- A submodule checkout is a detached HEAD. Run `git switch <branch>` before committing in it.
- Commit and push only when explicitly asked.

## Common operations

```bash
# fresh clone
git clone --recursive https://github.com/karel-brinda/sshash-family-masked-superstrings
git submodule update --init --recursive

# sync a modification with its upstream (example: 12 ← jermp/sshash)
cd 12_sshash-bigreedy-ms
git switch master
git fetch upstream && git merge upstream/master   # or rebase, per tool
git submodule update --init --recursive
git push
cd .. && git -C 02_sshash fetch && git -C 02_sshash checkout <new fork point>
git add 02_sshash 12_sshash-bigreedy-ms && git commit

# status of everything
git submodule status --recursive
```

## Per-tool notes

<!-- To be backfilled: build instructions, CLI usage, design notes, benchmarks, gotchas. -->

### 11_sshash-lite-bigreedy-ms

TODO

### 12_sshash-bigreedy-ms

TODO

### 13_sshash-rs-bigreedy-ms

TODO
