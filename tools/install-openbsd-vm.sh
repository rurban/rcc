#!/bin/sh
# SPDX-License-Identifier: LGPL-2.1-or-later
# Download, boot, and register a local KVM-accelerated OpenBSD VM for
# fast, interactive iteration on OpenBSD-specific bugs -- CI's
# vmactions/openbsd-vm job (see .github/workflows/ci.yml) is accurate but
# a 10+ minute round trip per push; this VM gives full SSH/rebuild/iterate
# access using the exact same prebuilt disk image vmactions itself uses.
#
# Requires: qemu-system-x86_64 with KVM, ~4GB free disk, curl, zstd.
# Usage: ./tools/openbsd-vm-install.sh [start|stop|status]
#   (no args, or "start"): download the image if needed, then boot it
#   stop:                  shut down the running VM
#   status:                report whether the VM is up and SSH-reachable
set -e

VM_DIR="${OPENBSD_VM_DIR:-$HOME/bsdvm}"
VM_VER="${OPENBSD_VM_VER:-7.9}"
VM_REL="${OPENBSD_VM_REL:-v2.1.0}"
VM_BASE_URL="https://github.com/anyvm-org/openbsd-builder/releases/download/${VM_REL}"
SSH_PORT="${OPENBSD_VM_SSH_PORT:-2224}"
SSH_KEY="$VM_DIR/openbsd-${VM_VER}-host.id_rsa"
PID_FILE="$VM_DIR/qemu.pid"
QCOW2="$VM_DIR/openbsd-${VM_VER}.qcow2"

ssh_cmd() {
    ssh -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null \
        -o ConnectTimeout=3 -i "$SSH_KEY" -p "$SSH_PORT" root@127.0.0.1 "$@"
}

status() {
    if [ -f "$PID_FILE" ] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
        if ssh_cmd true 2>/dev/null; then
            echo "openbsd-vm: running (pid $(cat "$PID_FILE")), SSH reachable on 127.0.0.1:$SSH_PORT"
            return 0
        fi
        echo "openbsd-vm: process running (pid $(cat "$PID_FILE")) but SSH not (yet) reachable"
        return 1
    fi
    echo "openbsd-vm: not running"
    return 1
}

stop_vm() {
    if [ -f "$PID_FILE" ] && kill -0 "$(cat "$PID_FILE")" 2>/dev/null; then
        echo "==> Stopping openbsd-vm (pid $(cat "$PID_FILE"))..."
        kill "$(cat "$PID_FILE")"
        rm -f "$PID_FILE"
    else
        echo "openbsd-vm: not running"
    fi
}

case "${1:-start}" in
    stop) stop_vm; exit 0 ;;
    status) status; exit $? ;;
    start) ;;
    *) echo "usage: $0 [start|stop|status]" >&2; exit 1 ;;
esac

if ! command -v qemu-system-x86_64 >/dev/null 2>&1; then
    echo "error: qemu-system-x86_64 not found (install qemu-system-x86 / qemu-kvm)" >&2
    exit 1
fi
if [ ! -e /dev/kvm ]; then
    echo "error: /dev/kvm not available -- KVM acceleration is required" >&2
    exit 1
fi

mkdir -p "$VM_DIR/build"

if [ ! -f "$QCOW2" ]; then
    echo "==> Downloading OpenBSD $VM_VER prebuilt VM image (anyvm-org/openbsd-builder $VM_REL)..."
    curl -sSL -o "$VM_DIR/openbsd-${VM_VER}.qcow2.zst" "$VM_BASE_URL/openbsd-${VM_VER}.qcow2.zst"
    curl -sSL -o "$SSH_KEY" "$VM_BASE_URL/openbsd-${VM_VER}-host.id_rsa"
    chmod 600 "$SSH_KEY"
    curl -sSL -o "$VM_DIR/openbsd-${VM_VER}-id_rsa.pub" "$VM_BASE_URL/openbsd-${VM_VER}-id_rsa.pub"
    curl -sSL -o "$VM_DIR/openbsd-${VM_VER}.qemu" "$VM_BASE_URL/openbsd-${VM_VER}.qemu"
    echo "==> Decompressing image (this takes a minute)..."
    zstd -d -f -o "$QCOW2" "$VM_DIR/openbsd-${VM_VER}.qcow2.zst"
    rm -f "$VM_DIR/openbsd-${VM_VER}.qcow2.zst"
fi
ln -sf "../openbsd-${VM_VER}.qcow2" "$VM_DIR/build/openbsd.qcow2"

if status >/dev/null 2>&1; then
    echo "==> openbsd-vm already running"
    exit 0
fi

echo "==> Booting openbsd-vm (2 vCPU, 4GB RAM, SSH forwarded to 127.0.0.1:$SSH_PORT)..."
# Same invocation as the vendored openbsd-7.9.qemu descriptor, except
# "-display vnc=..." -> "-display none": headless while still routing the
# serial console correctly. "-nographic" instead silently breaks console
# routing and the boot hangs with no further serial output.
(
    cd "$VM_DIR"
    exec qemu-system-x86_64 \
        -chardev socket,id=serial0,host=127.0.0.1,port=7000,server=on,wait=off,logfile=build/openbsd.serial.log \
        -serial chardev:serial0 \
        -monitor tcp:127.0.0.1:4444,server,nowait,nodelay \
        -name openbsd -m 4096 -smp 2 \
        -rtc base=utc,clock=host,driftfix=slew \
        -netdev user,id=net0,net=192.168.122.0/24,host=192.168.122.1,dhcpstart=192.168.122.254,ipv6=off,"hostfwd=tcp:127.0.0.1:${SSH_PORT}-192.168.122.254:22" \
        -object rng-builtin,id=rng0 \
        -device virtio-rng-pci,rng=rng0,max-bytes=1024,period=1000 \
        -machine pc,accel=kvm,hpet=off,smm=off,graphics=on,vmport=off,usb=on \
        -cpu host,kvm=on,l3-cache=on,+hypervisor,migratable=no,+invtsc,pmu=off \
        -global kvm-pit.lost_tick_policy=delay \
        -device virtio-net-pci,netdev=net0 \
        -device virtio-balloon-pci \
        -drive file=build/openbsd.qcow2,format=qcow2,if=virtio,discard=unmap,detect-zeroes=unmap \
        -vga virtio -display none -device usb-tablet \
        >build/qemu.log 2>&1 &
    echo $! >"$PID_FILE"
)

echo "==> Waiting for SSH..."
i=0
while [ "$i" -lt 90 ]; do
    if ssh_cmd true 2>/dev/null; then
        echo "==> openbsd-vm is up. Try it with:"
        echo ""
        echo "  ssh -i $SSH_KEY -p $SSH_PORT root@127.0.0.1"
        echo ""
        echo "Or run the test suite against it with ./openbsd-test.sh"
        exit 0
    fi
    i=$((i + 1))
    sleep 2
done
echo "error: SSH did not come up within 3 minutes; check $VM_DIR/build/qemu.log" >&2
exit 1
