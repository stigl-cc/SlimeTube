if [ "$#" -lt 1 ]; then
    echo "Wrong number of arguments!"
    echo "Usage: ./headerize_query.sh source.sql"
    exit
fi

file=$1
output_file=${file%.*}.h
base_name=$(basename -- "$file");
query_name=${base_name%.*}

query_original=$(cat $1)
query_oneline=${query_original//$'\n'/}

query_counter=0

echo -e "#pragma once\n#include <array>\n#include <string_view>" > $output_file

IFS=';' read -ra ADDR <<< "$query_oneline"
length=${#ADDR[@]}
echo "constexpr std::array<std::string_view, $length> query_$query_name = {" >> $output_file
for i in "${ADDR[@]}"; do
    single_query="$i ;"
    query_counter=$((query_counter + 1))

    echo 'R"unique_token(' $single_query ')unique_token",' >> $output_file
done

echo "};" >> $output_file
