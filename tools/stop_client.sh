#!/bin/bash

screen_name=client
extension=".ini"

# only need to do something if the screen session is up
screen_exists=`screen -ls | grep -q $screen_name`
if ! screen -ls | grep -q $screen_name; then
    echo "No instances of p2pasclient running."
    exit 0;
fi

while (( `screen -ls | grep -c $screen_name` && `screen -S $screen_name -Q windows | wc -w` > 0 )); do
    windows=`screen -S $screen_name -Q windows | rev`
    field=1
    if [[ $windows = *"$"* ]]; then # at least one logged in session
        # `screen -Q windows` truncates the output, so we have to deal with that carefully
        # if no logged in session, no output truncation, hence field=1 above
        if [[ `echo $windows | wc -w` > 2 ]]; then
            if [[ `echo $windows | cut -d ' ' -f2` = *"$"* ]]; then
                field=3 # third entry from the end -- don't use the first one in case it was truncated
            else
                field=2
            fi
        fi
    fi
    pos=`echo $windows | cut -d ' ' -f$field | rev`
    echo Shutting down $pos.
    screen -S $screen_name -p $pos -X stuff $"^C exit\n"
    sleep 1
    if test -f "$pos$extension"; then
        rm "$pos$extension"
    fi
done

echo "Shut down all instances of p2pasclient."
exit 0;
