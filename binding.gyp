{
  "targets": [
    {
      "target_name": "addon",
      "sources": [
        "addon.cpp",
        "flac.cpp"
      ],
      "include_dirs": [
        ".", "./deps/flac/include/", "<!(node -e \"require('nan')\")"
      ],
      "libraries": [
        "-lFLAC++", "-L ./deps/flac/src/libFLAC++/.libs"
      ]
    }
  ]
}
