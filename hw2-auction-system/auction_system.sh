#!/bin/bash
n_host=$1
n_player=$2
#echo "$n_host $n_player"

#open all fifo for read, write
for ((i=0;i<=n_host;i++)); do
    fifo_name=fifo_$i.tmp
    mkfifo $fifo_name
    #echo -e "$fifo_name"
    eval exec $(($i+3))'<> $fifo_name'
    #echo -e "$(($i+3))"
    #rm -f $fifo_name
done
declare -a all_score
for ((i=0;i<=n_player;i++)); do
    all_score+=(0)
    #echo ${all_score[$i]}
done

#generate key
declare -a keys
keys=($(shuf -i 0-65536 -n $(($n_host+1)) ))
#echo "${keys[@]}"
#for ((i=0;i<n_host;i++)); do
#    keys+=($RANDOM)
#    #echo ${keys[$i]}
#done

#generate combination
cnt=1
for ((i=1;i<=n_player-7;i++)); do
    for((j=i+1;j<=n_player-6;j++)); do
        for((k=j+1;k<=n_player-5;k++)); do
            for((l=k+1;l<=n_player-4;l++)); do
                for((m=l+1;m<=n_player-3;m++)); do
                    for((n=m+1;n<=n_player-2;n++)); do
                        for((o=n+1;o<=n_player-1;o++)); do
                            for((p=o+1;p<=n_player;p++)); do
                                combination=($i $j $k $l $m $n $o $p)
                                #echo ${combination[@]}
                                if [[ cnt -le n_host ]]; then
                                    ./host $cnt ${keys[$cnt]} 0 &
                                    tarfifo=fifo_$cnt.tmp
                                    #echo $tarfifo
                                    echo ${combination[@]} > $tarfifo
                                else
                                    #read key to identify available host
                                    read -u 3 host_key
                                    for((n8=0;n8<8;n8++)); do
                                        read -u 3 player_id player_rank
					player_score=$((8-player_rank))
					((all_score[$player_id]+=player_score))
                                    done
                                    for((find_host=1;find_host<=n_host;find_host++)); do
                                        if [[ ${keys[$find_host]} -eq $host_key ]]; then
                                            tarfifo=fifo_$find_host.tmp
                                            #echo $tarfifo
                                            echo ${combination[@]} > $tarfifo
                                            break
                                        fi
                                    done
                                fi
                                ((cnt++))
                            done
                        done
                    done
                done
            done
        done
    done
done

ending_meg="-1 -1 -1 -1 -1 -1 -1 -1"
#number of combinations might less than host!
if [[ $cnt -le $n_host ]]; then
	((final_run=cnt-1))
else
	final_run=$n_host
fi
for ((i=1;i<=final_run;i++)); do
    read -u 3 host_key
    for((n8=0;n8<8;n8++)); do
        read -u 3 player_id player_rank
	player_score=$((8-player_rank))
	((all_score[$player_id]+=player_score))
    done
    for((find_host=1;find_host<=n_host;find_host++)); do
        if [[ ${keys[$find_host]} -eq $host_key ]]; then
            tarfifo=fifo_$find_host.tmp
            #echo $tarfifo
            echo $ending_meg > $tarfifo
            break
        fi
    done
done
for ((i=1;i<=n_player;i++)); do
    echo $i ${all_score[$i]}
    #tarout=out_${n_host}_${n_player}
    #echo -n $i ${all_score[$i]} "\n" > $tarout
done

rm fifo*
wait

exit 0
