# my-file-system
一个简单的文件系统
# 使用方法
cd：进入指定路径下的目录    
 
pwd：查看当前所在路径

mkdir：删除指定路径下的文件夹    
例如:mkdir Dir1/test1.txt

rmdir：删除指定路径下的目录      
例如:rmdir Dir1/test1.txt

rm：删除文件，path为指定文件/文件夹，op表示删除方式，op==0表示放入回收站的删除，op==1表示强制删除不放入回收站    
例如:
rm Dir2/test2.txt 0（在指定目录下珊瑚后放入回收站，只能删除文件）
rm Dir2（在指定目录下珊瑚后不放入回收站，既可以删除文件也可以删除文件夹）

cat：查看指定路径下的文件内容
例如:cat Dir1/test1.txt

write：向指定路径下的文件写内容。以'\`'字符作为结束标志
write Dir1/test1.txt
print(“hello world”)
\`
exit：退出文件系统
