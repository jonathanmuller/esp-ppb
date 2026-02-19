#!/bin/bash
export IDF_PATH_FORCE=1
export IDF_PATH=~/esp/esp-idf && . $HOME/esp/esp-idf/export.sh

trigger_build_and_flash() {
    echo "------------------------------------------------"
    echo "Triggered! Building project..."
    idf.py build

    if [ $? -eq 0 ]; then
        echo "Build successful. Killing all 'idf.py flash monitor' processes..."
        pkill -f "idf.py flash monitor"
        # Also kill the underlying python monitor script which actually holds the port
        pkill -f "idf_monitor.py"
        echo "Done. All upload.sh instances should now wake up and re-flash."
    else
        echo "Build failed! Not killing monitors."
    fi
}

if [ -z "$1" ]; then
    # No argument: Run once
    trigger_build_and_flash
else
    # Argument provided: Watch mode
    if ! command -v inotifywait &> /dev/null; then
        echo "Error: 'inotifywait' is not installed. Install with: sudo apt install inotify-tools"
        exit 1
    fi

    echo "listening to file changes (excluding build/ and .git/)..."
    # Use -m to monitor continuously. Pipe to loop to process events without missing any.
    inotifywait -m -q -r -e modify,create,delete,move \
        --exclude '(\.git/|build/|cmake-build-debug/|\.idea/)' \
        --format '%w%f' \
        . | while read FILE_CHANGED; do
        
        # Only trigger for .c or .h files
        if [[ "$FILE_CHANGED" =~ \.(c|h)$ ]]; then
            echo "File changed: $FILE_CHANGED"
            
            # Small delay to allow "Save All" to settle
            sleep 0.5
            
            trigger_build_and_flash
            
            # Debounce: Drain any events that accumulated during the build
            while read -t 1 SKIPPED; do
                :
            done
        fi
    done
fi
