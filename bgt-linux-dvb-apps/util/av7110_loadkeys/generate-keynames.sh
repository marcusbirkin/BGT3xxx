# Makefile helper for linuxtv.org dvb-apps/util/av7110_loadkeys

echo "generate $1..."
echo "#ifndef INPUT_KEYNAMES_H" > $1
echo "#define INPUT_KEYNAMES_H" >> $1
echo >> $1
echo "#include <linux/input.h>" >> $1
echo >> $1
echo "#if !defined(KEY_OK)" >> $1
echo "#include \"input_fake.h\"" >> $1
echo "#endif" >> $1
echo >> $1
echo >> $1
echo "struct input_key_name {" >> $1
echo "        const char *name;" >> $1
echo "        int         key;" >> $1
echo "};" >> $1
echo >> $1
echo >> $1
echo "static struct input_key_name key_name [] = {" >> $1
for x in $(cat /usr/include/linux/input.h input_fake.h | \
           egrep "#define[ \t]+KEY_" | grep -v KEY_MAX | \
           cut -f 1 | cut -f 2 -d " " | sort -u) ; do
    echo "        { \"$(echo $x | cut -b 5-)\", $x }," >> $1
done
echo "};" >> $1
echo >> $1
echo "static struct input_key_name btn_name [] = {" >> $1
for x in $(cat /usr/include/linux/input.h input_fake.h | \
           egrep "#define[ \t]+BTN_" | \
           cut -f 1 | cut -f 2 -d " " | sort -u) ; do
     echo "        { \"$(echo $x | cut -b 5-)\", $x }," >> $1
done
echo "};" >> $1
echo >> $1
echo "#endif /* INPUT_KEYNAMES_H */" >> $1
echo >> $1
