#!/bin/sh
# Sync the current source tree to the local FreeBSD VM (see
# tools/install-freebsd-vm.sh) and build+run tests there natively --
# much faster iteration than round-tripping through CI's
# vmactions/freebsd-vm job for FreeBSD-specific bugs.
# Usage: ./freebsd-test.sh [test-name...]
#   (no args): full suite, mirroring the CI job (check + test-torture)
#   test-name: build once, then `./run_tests ./rcc test-name...`
set -e

SCRIPT_DIR="$(CDPATH='' cd -- "$(dirname -- "$0")" && pwd)"
VM_DIR="${FREEBSD_VM_DIR:-$HOME/bsdvm}"
VM_VER="${FREEBSD_VM_VER:-7.9}"
SSH_PORT="${FREEBSD_VM_SSH_PORT:-2224}"
SSH_KEY="$VM_DIR/freebsd-${VM_VER}-host.id_rsa"
REMOTE_DIR="${FREEBSD_VM_REMOTE_DIR:-/root/rcc}"
SSH_OPTS="-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -i $SSH_KEY -p $SSH_PORT"
SCP_OPTS="-o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -i $SSH_KEY -P $SSH_PORT"

if ! "$SCRIPT_DIR/tools/install-freebsd-vm.sh" status >/dev/null 2>&1; then
    "$SCRIPT_DIR/tools/install-freebsd-vm.sh" start
fi
# shellcheck disable=SC2086,SC2029  # SSH_OPTS word-split intentional; "$@" is
# always one already-built command string, client-side expansion intended
sshx() { ssh $SSH_OPTS root@127.0.0.1 "$@"; }
# shellcheck disable=SC2086
scpx() { scp $SCP_OPTS "$@"; }

echo "==> Syncing tracked source to $REMOTE_DIR on the VM..."
# A plain rsync of the working tree is slow and drags in stray build
# artifacts; a git-ls-files tarball is small (~10MB) and clean.
TARBALL="$(mktemp /tmp/rcc-freebsd-src.XXXXXX.tar.gz)"
trap 'rm -f "$TARBALL"' EXIT
git -C "$SCRIPT_DIR" ls-files -z | tar --null -T - -czf "$TARBALL" -C "$SCRIPT_DIR"
sshx "mkdir -p $REMOTE_DIR"
scpx "$TARBALL" "root@127.0.0.1:$REMOTE_DIR/../rcc-src.tar.gz"
sshx "cd $REMOTE_DIR && tar xzf ../rcc-src.tar.gz"

echo "==> Ensuring gmake is installed on the VM..."
sshx "command -v gmake >/dev/null 2>&1 || pkg_add gmake"

echo "==> Building rcc (gmake CC=cc)..."
sshx "cd $REMOTE_DIR && gmake CC=cc"

if [ "$#" -gt 0 ]; then
    echo "==> Running: ./run_tests ./rcc $*"
    sshx "cd $REMOTE_DIR && ./run_tests ./rcc $*"
    exit $?
fi

echo "==> Running full test suite (check + test-torture)..."
FAIL=
sshx "cd $REMOTE_DIR && gmake CC=cc check" || FAIL=1
sshx "cd $REMOTE_DIR && gmake CC=cc test-torture" || FAIL=1

echo "==> Fetching reports into test/..."
scpx "root@127.0.0.1:$REMOTE_DIR/test/tcc_test_Freebsd.md" "$SCRIPT_DIR/test/" 2>/dev/null || true
scpx "root@127.0.0.1:$REMOTE_DIR/test/torture_report_Freebsd.log" "$SCRIPT_DIR/test/" 2>/dev/null || true
scpx "root@127.0.0.1:$REMOTE_DIR/test_report_Freebsd.md" "$SCRIPT_DIR/" 2>/dev/null || true

test -z "$FAIL"
