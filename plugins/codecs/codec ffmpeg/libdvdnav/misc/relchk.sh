#!/bin/sh
##
## A simple compare directory content utility.
##

topdir="`pwd`"
distdir="libdvdnav-0.1.10"
log="$topdir/dist-log"
logerror="$topdir/dist-errors"

getdir() {
  if test -r .relignore; then
    filelist=`ls | grep -Fxvf .relignore`
  else
    filelist=`ls`
  fi
  
  for file in $filelist; do

    if test -d $file -a $file != "CVS" -a $file != $distdir; then
      (cd $file && getdir) || (cd ..)
    else
      if test ! -d $file -a \
              $file != $log -a \
              $file != $logerror -a \
              $file != "$distdir.tar.gz"; then

        orifile=`pwd`/$file

        distfile=$topdir/$distdir${orifile##*$topdir}

        echo -e "check:\t$orifile\nand\t$distfile" >> $log

        if test ! -e $distfile; then
          missingfile=${orifile##$topdir}
	  echo "${missingfile#/} is missing in tarball" >> $logerror
        fi

      fi
    fi

  done
}

main() {
  rm -f $log $logerror

  make config.status && make dist && mv $distdir.tar.gz $distdir.tmp.tar.gz && \
  cp config.status config.tmp.status && make clean && make distclean && \
  mv $distdir.tmp.tar.gz $distdir.tar.gz && mv config.tmp.status config.status && \
  tar -xzf $distdir.tar.gz

  echo "Check is running, be patient..."
  getdir

  rm -rf $distdir
  rm -f $distdir.tar.gz
  ./config.status --recheck
  ./config.status

  echo " * Log is ${log##*/}"
  echo " * Error log is ${logerror##*/}"
  
}

main
