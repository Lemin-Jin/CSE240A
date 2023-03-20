#! /bin/bash
rm result.txt
touch result.txt
echo -e "static result\n" >> result.txt 
for i in fp_1 fp_2 int_1 int_2 mm_1 mm_2
do 
    ./predictor --static $i >> result.txt
done
echo -e "\n gshare result\n" >> result.txt 
for j in 5 10 15 20 25
do 
    echo "result with $j" >> result.txt
    for i in fp_1 fp_2 int_1 int_2 mm_1 mm_2
    do 
        ./predictor --gshare:$j $i >> result.txt
        echo -e "\n" >> result.txt
    done
done

echo -e "\n tournament result\n" >> result.txt 
for gshare_bit in 5 20
do 
    for local_bit in 5 20
    do 
        for index_bit in 5 20
        do
            for file in fp_1 fp_2 int_1 int_2 mm_1 mm_2
            do
                echo -e "below is the tournament result for $gshare_bit : $local_bit : $index_bit \n" >> result.txt
                ./predictor --tournament:$gshare_bit:$local_bit:$index_bit $file >> result.txt
                echo -e "\n" >> result.txt
            done
        done
    done  
done

