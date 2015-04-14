{
    "targets": [
        {
            "target_name":  "jack",
            "sources":      [ "jack.cc" ],
            "libraries":    [ "-ljack", "-luv" ],
            "include_dirs": [ "<!(node -e \"require('nan')\")" ]
        }
    ],
}
