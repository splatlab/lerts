#!/bin/bash 

TEMPFILE=$(tempfile)    # create temp file for results

trap "rm $TEMPFILE; exit 1" SIGINT  # cleanup after Ctrl+C

SECONDS=0               # reset timer

$@ &                    # execute command in background

IO=/proc/$!/io          # io data of command
while [ -e $IO ]; do
    cat $IO > "$TEMPFILE"   # "copy" data
    sed 's/.*/& Bytes/' "$TEMPFILE" | column -t
    echo
    sleep 1
done

S=$SECONDS              # save timer

echo -e "\nPerformace after $S seconds:"
while IFS=" " read string value; do
    echo "Total" $string $(($value/1024/1024/$S)) MByte/s
done < "$TEMPFILE" | column -t

rm "$TEMPFILE"          # remove temp file
