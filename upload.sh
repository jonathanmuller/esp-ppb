export IDF_PATH_FORCE=1
export IDF_PATH=~/esp/esp-idf && . $HOME/esp/esp-idf/export.sh
while true; do
    sleep 1
    # Force kill anything holding this specific port to prevent "Resource busy" errors
    # Try fuser (common on Linux)
    if command -v fuser >/dev/null 2>&1; then
        fuser -k /dev/ttyACM$1 >/dev/null 2>&1
    fi
    
    idf.py flash monitor --port /dev/ttyACM$1 --timestamps | tee /tmp/minicom$1.txt
    echo "I will upload in 1s"
    sleep 1
done