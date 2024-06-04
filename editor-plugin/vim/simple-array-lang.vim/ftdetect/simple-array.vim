" vint: -ProhibitAutocmdWithNoGroup

autocmd BufRead,BufNewFile *.sipa
            \   if &ft !=# 'simple-array'
            \ |     setf simple-array
            \ | en
