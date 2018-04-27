
#!/bin/bash  

DIR=`pwd`

BACKUP_FILE="${DIR}/mongo_dump.sh"

echo $BACKUP_FILE
tmp_file=`mktemp`

crontab -l > $tmp_file

number=`grep -n "$BACKUP_FILE" "$tmp_file" | cut -d ":" -f 1`
if [ -z $number ];then  
	echo "该任务不存在，将添加"  
	echo "1 12 * * * ${BACKUP_FILE}" >> $tmp_file  
else  
	echo "该任务已经存在，将会先删除再添加"  
	sed -i '/mongo_dump.sh/d' $tmp_file   
	echo "1 12 * * * ${BACKUP_FILE}" >> $tmp_file 
fi

cat $tmp_file
crontab $tmp_file   
rm -f $tmp_file  