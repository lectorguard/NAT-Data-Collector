#!/bin/bash

# Function to process trace
process_trace() {
    local trace_start=false
    while IFS= read -r line; do
        # Start processing only if we are within START-TRACE and END-TRACE
        if [[ "$line" =~ ^START-TRACE ]]; then
            trace_start=true
            echo "$line"
            continue
        elif [[ "$line" =~ ^END-TRACE ]]; then
            trace_start=false
            echo "$line"
            continue
        fi

        if $trace_start; then
            left_path=$(echo "$line" | awk -F'[()]' '{print $1}')
            address=$(echo "$line" | awk -F'[()]' '{print $2}' | awk -F' ' '{print $1}')
            bracket_content=$(echo "$line" | awk -F'[][]' '{print $2}')

            if [[ "$address" == +* ]]; then
                # Remove the leading '+'
                clean_address="${address#+}"
                result=$(addr2line -e "$left_path" "$clean_address")
                if [[ $result == *"?"* ]]; then
                    echo "$line"
                else
                    echo "$result [$bracket_content]"
                fi
            else
                echo "$line"
            fi
        else
            echo "$line"
        fi
    done
}

# Read the input file or stdin
if [ -f "$1" ]; then
    cat "$1" | process_trace
else
    cat "Pass the log path as argument"
fi
