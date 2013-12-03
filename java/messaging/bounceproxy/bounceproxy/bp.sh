#!/bin/bash
BaseURL="http://localhost:8080/bounceproxy/channels/"
ChannelPrefix="mgtest."
NumChannels=1

usage ()
{
 echo "Simple JOYn-Bounceproxy Test Tool"
 echo "Base URL: $BaseURL"
 echo "usage: $0 [OPTS] [ARGS]"
 sed -n 's/^   \(.\))#\(.*\)$/ -\1 \2/p' < $0
 echo ""
}

while getopts Dn:lcdr:s:h opt; do
 case "$opt" in
   D)#  Debugging
      set -x ;;
   n)#NUM  set number of channels (must preceede command)
      NumChannels="$OPTARG"
      ;;

   l)#  List channels
      curl -s "$BaseURL"|grep "id: "
      ;;

   c)#  Create channel(s)
      for i in `seq 1 $NumChannels`; do
       curl -i -X POST -H "Content-type:application/json" "$BaseURL?ccid=$ChannelPrefix$i"
       echo
      done
      ;;
   d)#  Delete channel(s)
      for i in `seq 1 $NumChannels`; do
       curl -i -X DELETE -H "Content-type:application/json" "$BaseURL$ChannelPrefix$i"
       echo
      done
      ;;

   r)#CACHEDATE  Receive on channel
      while true; do
       curl -i -X GET -H "Content-type:application/json" -H "X-Cache-Date:$OPTARG" "$BaseURL$ChannelPrefix$NumChannels"
       echo
      done
      ;;

   s)#MSG  Send MSG on channel
      MSG="$OPTARG"
      [ "$MSG" = "." ] && MSG='{ "body" : "", "index" : 0, "ttl_absolute_ms" : 1329214616568}'
      #while true; do
       curl -i -X POST -H "Content-type:application/json" --data "$MSG" "$BaseURL$ChannelPrefix$NumChannels/message/"
       echo
      #done
      ;;

   h)#  Show help
      usage; exit 0 ;;
   *) usage; exit 1 ;;
 esac
done

shift $(($OPTIND - 1))

exit 0

