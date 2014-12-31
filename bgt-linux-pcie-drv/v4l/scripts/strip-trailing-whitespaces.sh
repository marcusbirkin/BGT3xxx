#!/bin/sh
# Strips trailing whitespace.  Leading spaces and spaces after tabs are
# converted to the equivalent sequence of tabs only.

# Use the option "fast" to only check files Hg thinks are new or modified.
# The option "manifest" will use Hg's manifest command to check all files
# under Hg revision control.
# Otherwise, all files under the linux tree are checked, except files in CVS
# directories and .cvsignore files. This is the historical behavior.


if [ "x$1" = "xfast" ]; then
	files="hg status -man"
elif [ "x$1" = "xqfast" ]; then
	files="hg status --rev -2 -man"
elif [ "x$1" = "xmanifest" ]; then
	files="hg manifest | cut '-d ' -f3"
else
	files="find linux -name CVS -prune -o -type f -not -name .cvsignore -print"
fi

for file in `eval $files`; do
	case "$file" in
		*.gif | *.pdf | *.patch)
			continue
			;;
	esac

	perl -ne '
	s/[ \t]+$//;
	s<^ {8}> <\t>;
	s<^ {1,7}\t> <\t>;
	while( s<\t {8}> <\t\t>g || s<\t {1,7}\t> <\t\t>g ) {};
	print' < "${file}" | \
		diff -u --label="$file" "$file" --label="$file" -
done
