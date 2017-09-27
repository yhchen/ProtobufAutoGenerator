# ProtobufAutoGenerator
protobuf auto generate file and header tools.


## commands:

[`-protoexe`] : proto executable file path (defualt is ./proto)

<`-indir`> : base *.proto files directory. The tool will recursive walks through the folder and finds all the `*.proto` files

<`-outdir`> : output directory. will automatic generat `*.pb.h` and `*.pb.cc` files to this path

[`-fullheader`] : generate one file include all `*.pb.h` files. this command often used in projects with precompiled headers.

[`-fullbody`] : generate one or more file include all `*.pb.cc` files. use this command if you don't want to change the project when add new `.proto` files.

[`fullbodysplit`] : Each `fullbody` file contains a specified number of files that are split into multiple `fullbody` files.

## example:

> auto generate all *.proto files from "./Protobuf" directory into "./Define/protobuf/" path:
>
>`ProtobufAutoGenerator -protoexe ./tools/protoc.exe -indir ./Protobuf/ -outdir ./Define/protobuf/`

> full example:
>
>`ProtobufAutoGenerator -protoexe ./tools/protoc.exe -indir ./Protobuf/ -outdir ./Define/protobuf/ -fullheader proto_all.h -fullbody proto_all.cc -fullbodysplit 100`
