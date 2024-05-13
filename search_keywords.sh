#!/bin/bash

if [ $# -lt 1 ]; then
    echo "Usage: $0 <file(s)>"
    exit 1
fi

keywords=("corrupted" "dangerous" "risk" "attack" "malware" "malicious")
files=("$@")
exit_code=0

# Setează permisiuni pentru a citi fișierele
for file_name in "${files[@]}"; do
    chmod 777 "$file_name"
done

# Calculăm numărul de linii, cuvinte și caractere
num_lines=$(wc -l < "$1")
num_words=$(wc -w < "$1")
num_chars=$(wc -m < "$1")

# Caută cuvinte cheie și caractere non-ASCII
if [ "$num_lines" -lt 3 ] && [ "$num_words" -gt 1000 ] && [ "$num_chars" -gt 2000 ]; then
    for file_name in "${files[@]}"; do
        if [ -f "$file_name" ]; then
            for keyword in "${keywords[@]}"; do
                if grep -i -q "$keyword" "$file_name"; then
                    exit_code=1
                fi
            done

            if grep -P -q '[^\x00-\x7F]' "$file_name"; then
                exit_code=1
            fi
        else
            echo "File $file_name does not exist or is not a regular file"
        fi
    done
fi

# Resetează permisiunile înapoi la 0
for file_name in "${files[@]}"; do
    chmod 000 "$file_name"
done

# Returnează codul de ieșire
echo -n "$exit_code"
