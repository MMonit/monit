# bash completion for monit(1)
#
# This file is installed to <prefix>/share/bash-completion/completions/ by
# "make install", from where the bash-completion package loads it on demand
# when completing the monit command.
#
# It uses bash builtins only and doesn't depend on the bash-completion
# package, so it can be sourced from ~/.bashrc as well:
#
#     source /path/to/monit.bash

# Remove the quoting from a word of the command line and expand a leading
# tilde. Nothing is evaluated.
# @param $1  Word
# @var[out] REPLY
_monit_dequote()
{
    REPLY=$1
    case $REPLY in
        \"*\" | \'*\')
            REPLY=${REPLY:1:${#REPLY}-2}
            ;;
        *)
            REPLY=${REPLY//\\/}
            ;;
    esac
    case $REPLY in
        \~) REPLY=$HOME ;;
        \~/*) REPLY=$HOME/${REPLY#\~/} ;;
    esac
}

# Add the names matching the current word to COMPREPLY.
# @param $@  Names
_monit_reply()
{
    local IFS=$'\n'
    local name
    for name in "$@"; do
        [[ $name ]] || continue
        printf -v name %q "$name"
        [[ $name == "$cur"* ]] || continue
        # Skip duplicates
        [[ $'\n'${COMPREPLY[*]-}$'\n' == *$'\n'$name$'\n'* ]] && continue
        COMPREPLY+=("$name")
    done
}

# Complete file names.
_monit_files()
{
    local line
    compopt -o filenames 2>/dev/null
    while IFS= read -r line; do
        [[ $line ]] && COMPREPLY+=("$line")
    done <<<"$(compgen -f -- "$cur")"
}

# Complete the names of services or service groups defined in the control
# file.
# @param $1  "services" or "groups"
_monit_names()
{
    local re_service='^[A-Z][A-Za-z ]* Name +=[ ](.*)$'
    local re_group='^ Group +=[ ](.*)$'
    local line group
    local -a names=() groups=()
    while IFS= read -r line; do
        if [[ $1 == services && $line =~ $re_service ]]; then
            names+=("${BASH_REMATCH[1]}")
        elif [[ $1 == groups && $line =~ $re_group ]]; then
            IFS=, read -r -a groups <<<"${BASH_REMATCH[1]}"
            for group in "${groups[@]}"; do
                names+=("${group# }")
            done
        fi
    done <<<"$("$monit" ${conffile:+-c "$conffile"} -vIt 2>/dev/null)"
    _monit_reply "${names[@]-}"
}

# Complete the names of running processes.
_monit_processes()
{
    local line
    local -a names=()
    while IFS= read -r line; do
        line=${line##*/}
        names+=("${line#-}")
    done <<<"$(command ps -A -o comm= 2>/dev/null)"
    _monit_reply "${names[@]-}"
}

_monit()
{
    local IFS=$' \t\n'
    local cur=${COMP_WORDS[COMP_CWORD]}
    local monit conffile="" group="" command="" optarg="" split=""
    local REPLY word opts i n
    local -a words=()

    COMPREPLY=()

    _monit_dequote "$1"
    monit=$REPLY

    # The words before the cursor, "--option=value" is split to "--option",
    # "=" and "value" in COMP_WORDS
    for ((i = 1; i < COMP_CWORD; i++)); do
        [[ ${COMP_WORDS[i]} == = ]] || words+=("${COMP_WORDS[i]}")
    done
    if [[ $cur == = ]]; then
        cur=""
        split="set"
    elif ((COMP_CWORD > 1)) && [[ ${COMP_WORDS[COMP_CWORD - 1]} == = ]]; then
        split="set"
    fi

    # Find the option which takes the current word as its argument, the
    # control file and the command
    n=${#words[@]}
    for ((i = 0; i < n; i++)); do
        word=${words[i]}
        optarg=""
        case $word in
            --conf) optarg=c ;;
            --daemon) optarg=d ;;
            --group) optarg=g ;;
            --logfile) optarg=l ;;
            --pidfile) optarg=p ;;
            --statefile) optarg=s ;;
            --hash | -H) optarg=H ;;
            --*) ;;
            -?*)
                # Short options, possibly bundled, e.g. "-vc file" or "-cfile"
                opts=${word#-}
                while [[ $opts ]]; do
                    case $opts in
                        [cdglps])
                            optarg=$opts
                            ;;
                        c?*)
                            _monit_dequote "${opts:1}"
                            conffile=$REPLY
                            ;;
                        g?*)
                            group=${opts:1}
                            ;;
                    esac
                    [[ $opts == [cdglps]* ]] && break
                    opts=${opts:1}
                done
                ;;
            *)
                # Options are recognized before the command only
                command=$word
                break
                ;;
        esac
        if [[ $optarg ]] && ((++i < n)); then
            if [[ $optarg == c ]]; then
                _monit_dequote "${words[i]}"
                conffile=$REPLY
            elif [[ $optarg == g ]]; then
                group=${words[i]}
            fi
            optarg=""
        fi
    done

    if [[ $command ]]; then
        # One argument per command
        ((i == n - 1)) || return 0
        case $command in
            start | stop | restart | monitor | unmonitor)
                # The action applies to the group if specified
                [[ $group ]] && return 0
                _monit_names services
                _monit_reply all
                ;;
            status | summary)
                _monit_names services
                ;;
            report)
                _monit_reply up down initialising unmonitored total
                ;;
            procmatch)
                _monit_processes
                ;;
        esac
        return 0
    fi

    case $optarg in
        c | p | s | H)
            _monit_files
            return 0
            ;;
        l)
            _monit_files
            _monit_reply syslog
            return 0
            ;;
        g)
            _monit_names groups
            return 0
            ;;
        d)
            return 0
            ;;
    esac

    [[ $split ]] && return 0

    if [[ $cur == -* ]]; then
        _monit_reply --batch --conf --daemon --group --hash --help --id \
            --interactive --logfile --pidfile --resetid --statefile --test \
            --verbose --version
        return 0
    fi

    _monit_reply start stop restart monitor unmonitor reload status summary \
        report inventory quit validate procmatch
    return 0
}

complete -F _monit monit
