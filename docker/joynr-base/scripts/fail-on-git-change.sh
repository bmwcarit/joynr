CHANGES=`git status -uno --porcelain 2>/dev/null | wc -l`
      if [ "$CHANGES" != "0" ]; then
          echo 'ERROR: CHANGES FOUND -> BUILD FAILED'
          git status
          git diff --color --exit-code | cat
          exit 1
      fi