#!/bin/sh

# commit all debian patches

set -e
set -u

test -d debian/patches

mkdir -p debian_done

for p in $( cat debian/patches/series ) ; do
	echo "applying $p"
	git apply --index debian/patches/"$p"
	title=$( echo "$p" | sed -e 's/\(\.patch\)\|\(\.diff\)\|\(.dpatch\)$//' | perl -pe 's/^.*?_//' | sed -e 's/_/ /g' )

	( echo "$title"
	  echo
	  sed -n '0,/^--- /p' < debian/patches/"$p" \
		| head -n -1 \
		| grep -v '^diff ' \
		| grep -v '^index ' \
		| sed '${/^$/d;}' \
		| sed '${/^$/d;}' \
		| sed '${/^$/d;}'
	  echo
	  echo "Debian patch: $p"
	) | git commit -F - --author='Friendly Debian Maintainer <none@example.com>'
	sleep 1 # for better timestamps
done
