"""Kopiert geklonte Originalcode-Repos in comdare-cache-engine/ext/.

(a) Phase 4.B-detail: 12 Repos ohne .git aus Forschungsarbeiten/code/ in ext/.
"""
import shutil
from pathlib import Path

SOURCE_ROOT = Path(r"C:\Users\benja\OneDrive\Desktop\Diplomarbeit - Datenbanken\Forschungsarbeiten\code")
TARGET_ROOT = Path(__file__).parent / "ext"

# Mapping: P-ID-Verzeichnis → Liste der zu kopierenden Repo-Subverzeichnisse
REPOS = {
    "P01-ART": ["unodb"],
    "P02-HOT": ["hot"],
    "P03-Masstree": ["masstree-beta"],
    "P04-CoCo-trie": ["CoCo-trie"],
    "P05-START": ["START"],
    "P06-B2tree": ["b2-tree-master", "bart-master"],
    "P07-Wormhole": ["wormhole"],
    "P10-SuRF": ["SuRF"],
    "P20-BTreesAreBack": ["leanstore"],
    "P25-Mahling-FillBuffer": ["prefetching"],
    "P29-RCU-McKenney": ["userspace-rcu"],
    "P30-Hazard-Pointers": ["haz_ptr"],
}

# Mapping fuer ext/-Zielnamen (kuerzer)
TARGET_NAMES = {
    "P01-ART": "P01-ART",
    "P02-HOT": "P02-HOT",
    "P03-Masstree": "P03-Masstree",
    "P04-CoCo-trie": "P04-CoCo-trie",
    "P05-START": "P05-START",
    "P06-B2tree": "P06-B2tree",
    "P07-Wormhole": "P07-Wormhole",
    "P10-SuRF": "P10-SuRF",
    "P20-BTreesAreBack": "P20-BTreesAreBack",
    "P25-Mahling-FillBuffer": "P25-Mahling",
    "P29-RCU-McKenney": "P29-RCU",
    "P30-Hazard-Pointers": "P30-HazardPointers",
}


def ignore_git(_dir, names):
    """Excludiert .git-Verzeichnisse beim Kopieren."""
    return [n for n in names if n in (".git", ".github")]


def copy_repos():
    total_copied = 0
    for source_pid, repos in REPOS.items():
        target_pid = TARGET_NAMES[source_pid]
        target_dir = TARGET_ROOT / target_pid
        target_dir.mkdir(parents=True, exist_ok=True)
        for repo in repos:
            src = SOURCE_ROOT / source_pid / repo
            dst = target_dir / repo
            if not src.exists():
                print(f"[SKIP] {src} existiert nicht")
                continue
            if dst.exists():
                print(f"[SKIP] {dst} existiert schon")
                continue
            print(f"[COPY] {src.name} -> {dst}")
            shutil.copytree(src, dst, ignore=ignore_git)
            total_copied += 1
    print(f"OK: {total_copied} Repos kopiert.")


if __name__ == "__main__":
    copy_repos()
