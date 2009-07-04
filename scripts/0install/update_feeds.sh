#!/bin/sh

csdir=`dirname $0`/../..
csbindir=$csdir/bin

csver_major=`cat $csdir/include/csver.h | grep "^#define *CS_VERSION_NUM_MAJOR" | sed -e "s/[^0-9]*//"`
csver_minor=`cat $csdir/include/csver.h | grep "^#define *CS_VERSION_NUM_MINOR" | sed -e "s/[^0-9]*//"`
csver_build=`cat $csdir/include/csver.h | grep "^#define *CS_VERSION_NUM_BUILD" | sed -e "s/[^0-9]*//"`
csver_svnrev=`svn info $csdir | grep "^Last Changed Rev: " | sed -e "s/^[^0-9]*//"`
CSVER=$csver_major.$csver_minor.$csver_build.$csver_svnrev
FEEDVER=$csver_major.$csver_minor

create_archive()
{
  feedprefix=$1
  shift
  lists=$*
  
  archive=$feedprefix-$CSVER
  $csbindir/archive-from-lists.sh $archive $lists
}

_update_feed()
{
  feedprefix=$1
  arch=$2
  download_dir=$3
  
  archive=$feedprefix-$CSVER
  archive_url=http://crystalspace3d.org/downloads/binary/$FEEDVER/${download_dir}$archive.tar.lzma
  feedname=$feedprefix-$FEEDVER
  feedpath=$csdir/scripts/0install/$feedname.xml
  
  if [ -e $feedpath ] ; then
    0publish -u $feedpath
    0publish	\
      --add-version=$CSVER	\
      --archive-url=$archive_url	\
      --archive-file=$archive.tar.lzma	\
      --archive-extract=$archive	\
      --set-arch=$arch	\
      --set-stability=testing	\
      --set-released=`date +%Y-%m-%d`	\
      $feedpath
  else
    0publish -c $feedpath
    0publish	\
      --set-version=$CSVER	\
      --archive-url=$archive_url	\
      --archive-file=$archive.tar.lzma	\
      --archive-extract=$archive	\
      --set-arch=$arch	\
      --set-stability=testing	\
      --set-released=`date +%Y-%m-%d`	\
      $feedpath
    echo "* A new 0install feed was created; you will have to adjust it's contents"
  fi
    
  echo "* Upload $archive.tar.lzma to $archive_url ."
  echo "* Don't forget to re-sign the feed file!"
  echo "     0publish -x -k <gpgkey> $feedpath"
}

update_feed()
{
  arch=`uname`-`uname -m`
  arch_lc=`echo $arch | tr "[:upper:]" "[:lower:]"`
  _update_feed $1 $arch $arch_lc/
}

update_feed_neutral()
{
  _update_feed $1 "*-*" ""
}

fixup_feed()
{
  feedprefix=$1
  feedname=$feedprefix-$FEEDVER
  feedpath=$csdir/scripts/0install/$feedname.xml
  
  # The *-sdk* feeds use a self-dependency to set the CRYSTAL_1_4 env var
  # 0publish doesn't have a command for that, so inject manually
  cat $feedpath | sed -e "s@\(<implementation[^>]*arch=\"$arch\"[^>]*version=\"$CSVER\">\)@\1\n\
      <requires interface=\"http://crystalspace3d.org/0install/$feedname.xml\">\n\
	    <environment insert=\"\" mode=\"prepend\" name=\"CRYSTAL_1_4\"/>\n\
      </requires>@g" > $feedpath.tmp
  mv $feedpath.tmp $feedpath
}

jam filelists

ALL_FEEDS="$1"
check_feed()
{
  feed_name=`echo $1 | tr -c "[\na-zA-Z]" "\n_"`
  use=0
  if [ -z "$ALL_FEEDS" ]; then
    use=1
  else
    for f in $ALL_FEEDS; do
      if [ $f = $1 ]; then
	use=1
	break
      fi
    done
  fi
  export feed_$feed_name=$use
}

check_feed libs
check_feed sdk
check_feed sdk-staticplugins
check_feed data
check_feed docs-manual
check_feed python

do_archive()
{
  feed_name=`echo $1 | tr -c "[\na-zA-Z]" "\n_"`
  if (( $(eval echo \$feed_$feed_name) )) ; then
    create_archive crystalspace-$1 $*
  fi
}

do_archive libs libs-shared@lib
# SDKs: we need to lump everything together
# splitting things over multiple feeds works, but is less robust
# (e.g. cs-config won't find the libs dir when invoked w/o 0launch)
SDK_LISTS="libs-static@lib libs-shared@lib cs-config@bin bin-tool@bin headers@include headers-platform@include"
do_archive sdk $SDK_LISTS
do_archive sdk-staticplugins $SDK_LISTS libs-staticplugins@lib
do_archive data data-runtime@data vfs
do_archive docs-manual doc-manual doc-util-open
do_archive python python-modules@py python-bindings@py plugin-python@plugins

do_update()
{
  feed_name=`echo $1 | tr -c "[\na-zA-Z]" "\n_"`
  if (( $(eval echo \$feed_$feed_name) )) ; then
    if [ "$2" = "neutral" ] ; then
      update_feed_neutral crystalspace-$1data
    else
      update_feed crystalspace-$1
    fi
    if [ "$2" = "fixup" ] ; then
      fixup_feed crystalspace-$1
    fi
  fi
}

do_update libs
do_update sdk fixup
do_update sdk-staticplugins fixup
do_update data neutral
do_update docs-manual neutral
do_update python
