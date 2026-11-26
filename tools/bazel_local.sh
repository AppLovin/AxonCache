# Sourced by build.sh: bb-portal build labels, local reporting, shared disk cache
# and the daily tidy.
#
# Opt-outs for local builds:
#   AL_BB_REPORT=0          do not report to bb-portal
#   AL_BAZEL_DISK_CACHE=0   do not use the shared disk cache
#   AL_BAZEL_TIDY=0 | dry   skip the daily tidy, or only print what it would do
#
# The AL_BB_* variables and BUILD_URL are read only by bb-portal's metadata extractor; no rule
# or --action_env reads them, so they do not change cache keys. Every probe must
# tolerate failure: build.sh runs under set -e (and sometimes pipefail).

AL_BB_BES_URL="https://buildbarn-bes.axon.tools"
AL_BB_PORTAL_URL="https://buildbarn-portal.axon.tools"
AL_BAZEL_DISK_CACHE_DIR="$HOME/.cache/bazel-disk-cache"

# Prints the command's output, or nothing if it fails.
_bazel_local_probe ()
{
    "$@" 2>/dev/null || true
}

_bazel_local_clang_version ()
{
    { clang --version 2>/dev/null || true; } | sed -n 's/.*clang version \([0-9.]*\).*/\1/p' | head -n 1
}

_bazel_local_machine_labels ()
{
    if [[ $(uname -s) == Darwin ]]; then
        AL_BB_CHIP="$(_bazel_local_probe sysctl -n machdep.cpu.brand_string)"
        AL_BB_MODEL="$(_bazel_local_probe sysctl -n hw.model)"
        AL_BB_CORES="$(_bazel_local_probe sysctl -n hw.ncpu)"
        AL_BB_PCORES="$(_bazel_local_probe sysctl -n hw.perflevel0.physicalcpu)"
        AL_BB_ECORES="$(_bazel_local_probe sysctl -n hw.perflevel1.physicalcpu)"
        local mem_bytes
        mem_bytes="$(_bazel_local_probe sysctl -n hw.memsize)"
        AL_BB_RAM_GB="$(( ${mem_bytes:-0} / 1073741824 ))"
        AL_BB_OS="macOS $(_bazel_local_probe sw_vers -productVersion)"
        AL_BB_SDK="$(_bazel_local_probe xcrun --show-sdk-version)"
        if [[ "$(_bazel_local_probe xcode-select -p)" == *Xcode* ]]; then
            AL_BB_TOOLCHAIN="xcode-clang-$(_bazel_local_clang_version)"
        else
            AL_BB_TOOLCHAIN="clt-clang-$(_bazel_local_clang_version)"
        fi
        case "$(_bazel_local_probe pmset -g batt)" in
            *"AC Power"*) AL_BB_POWER=ac ;;
            *"Battery Power"*) AL_BB_POWER=battery ;;
            *) AL_BB_POWER= ;;
        esac
    else
        AL_BB_CHIP="$(_bazel_local_probe sed -n 's/^model name[[:space:]]*: //p;T;q' /proc/cpuinfo)"
        AL_BB_MODEL=
        AL_BB_CORES="$(_bazel_local_probe nproc)"
        AL_BB_PCORES=
        AL_BB_ECORES=
        AL_BB_RAM_GB="$(_bazel_local_probe awk '/^MemTotal:/ {printf "%d", $2 / 1048576}' /proc/meminfo)"
        AL_BB_OS="$(_bazel_local_probe sed -n 's/^PRETTY_NAME="\{0,1\}\([^"]*\)"\{0,1\}$/\1/p' /etc/os-release)"
        AL_BB_SDK=
        AL_BB_TOOLCHAIN="$( { cc --version 2>/dev/null || true; } | head -n 1)"
        AL_BB_POWER=
    fi
    export AL_BB_CHIP AL_BB_MODEL AL_BB_CORES AL_BB_PCORES AL_BB_ECORES AL_BB_RAM_GB \
        AL_BB_OS AL_BB_SDK AL_BB_TOOLCHAIN AL_BB_POWER
}

# bazel_local_setup REPO EXEC_MODE BUILD_TYPE UNITY
#
# Exports the AL_BB_* labels for bb-portal (CI builds get build facts only) and,
# for local builds, sets BAZEL_LOCAL_ARGS to the extra Bazel flags:
# --config=local-disk-cache unless AL_BAZEL_DISK_CACHE=0, and --config=bb-report
# when reporting is on and the endpoint is reachable.
bazel_local_setup ()
{
    export AL_BB_REPO="$1" AL_BB_EXEC_MODE="$2" AL_BB_BUILD_TYPE="$3" AL_BB_UNITY="$4"
    AL_BB_COMMIT="$(_bazel_local_probe git rev-parse --short=12 HEAD)"
    # Argo checks out the commit SHA, so git reports HEAD; CI passes the branch in AL_BB_BRANCH.
    if [[ -z "${AL_BB_BRANCH:-}" ]]; then
        AL_BB_BRANCH="$(_bazel_local_probe git rev-parse --abbrev-ref HEAD)"
        [[ "$AL_BB_BRANCH" == "HEAD" ]] && AL_BB_BRANCH=
    fi
    export AL_BB_COMMIT AL_BB_BRANCH
    BAZEL_LOCAL_ARGS=

    if [[ "${CI:-}" == "true" ]]; then
        export AL_BB_ORIGIN=ci
        if [[ "${GITHUB_ACTIONS:-}" == "true" ]]; then
            export AL_BB_CI_SYSTEM=gha
        else
            export AL_BB_CI_SYSTEM=argo
        fi
        return 0
    fi

    export AL_BB_ORIGIN=local
    # bb-portal groups invocations into builds by BUILD_URL; keep one set by the caller.
    if [[ -z "${BUILD_URL:-}" ]]; then
        export BUILD_URL="local/${USER:-unknown}/$AL_BB_REPO/${AL_BB_BRANCH:-unknown}/$(date +%Y-%m-%d)"
    fi
    if [[ -n "$(_bazel_local_probe git status --porcelain --untracked-files=no)" ]]; then
        export AL_BB_DIRTY=yes
    else
        export AL_BB_DIRTY=no
    fi
    _bazel_local_machine_labels

    if [[ "${AL_BAZEL_DISK_CACHE:-1}" == "0" ]]; then
        export AL_BB_DISK_CACHE=off
    else
        export AL_BB_DISK_CACHE=on
        BAZEL_LOCAL_ARGS="--config=local-disk-cache"
    fi

    if [[ "${AL_BB_REPORT:-1}" == "0" ]]; then
        echo "bb-portal: reporting off (AL_BB_REPORT=0)"
    elif curl -s -o /dev/null --connect-timeout 2 "$AL_BB_BES_URL"; then
        BAZEL_LOCAL_ARGS="$BAZEL_LOCAL_ARGS --config=bb-report"
        echo "bb-portal: reporting this build to $AL_BB_PORTAL_URL (AL_BB_REPORT=0 to opt out)"
    else
        echo "bb-portal: $AL_BB_BES_URL not reachable (VPN off?), not reporting this build"
    fi
    return 0
}

# bazel_local_tidy BAZEL_BIN
#
# Local builds only, at most once a day: runs tools/bazel_tidy.py. Never fails.
bazel_local_tidy ()
{
    if [[ "${CI:-}" == "true" || "${AL_BAZEL_TIDY:-}" == "0" ]]; then
        return 0
    fi
    local stamp="$AL_BAZEL_DISK_CACHE_DIR/.last-tidy"
    if [[ -f "$stamp" && -z "$(_bazel_local_probe find "$stamp" -mmin +1440)" ]]; then
        return 0
    fi
    local script_dir install_base user_root output_base
    script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    # `bazel info` has no output_user_root key; install_base is <output_user_root>/install/<hash>.
    install_base="$(_bazel_local_probe "$1" info install_base)"
    output_base="$(_bazel_local_probe "$1" info output_base)"
    if [[ -z "$install_base" ]]; then
        return 0
    fi
    user_root="$(dirname "$(dirname "$install_base")")"
    python3 "$script_dir/bazel_tidy.py" --output-user-root "$user_root" \
        --current-output-base "$output_base" || true
    if [[ "${AL_BAZEL_TIDY:-}" != "dry" ]]; then
        mkdir -p "$AL_BAZEL_DISK_CACHE_DIR" && touch "$stamp" || true
    fi
    return 0
}
