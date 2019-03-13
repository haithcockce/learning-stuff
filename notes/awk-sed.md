# AWK and SED Notes

### AWK

##### General Use

`# awk '/REGEX1/BEG_PAT,END_PAT/REGEX2/ {Function}` FILE

The above says within the `BEG_PAT` and `END_PAT` range, look for `REGEX1` and 
`REGEX2`. When found, perform `Function` and consider the stuff between `REGEX1`
`REGEX2` as data to work on. 
