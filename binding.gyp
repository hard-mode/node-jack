{
    "targets": [
        {
            "target_name":  "jack",
            "sources":      [ "jack.cc" ],
            "libraries":    [ "-ljack" ],
            "include_dirs": [ "<!(node -e \"require('nan')\")" ]
        }
    ],
}
