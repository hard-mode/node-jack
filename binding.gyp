{
    "targets": [
        {
            "target_name":  "jack",
            "sources":      [ "src/jack.cc" ],
            "libraries":    [ "-ljack" ],
            "include_dirs": [ "<!(node -e \"require('nan')\")" ]
        }
    ],
}
