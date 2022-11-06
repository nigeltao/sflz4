# Single File LZ4

SFLZ4 is a [single file C
library](https://github.com/nothings/stb/blob/master/docs/stb_howto.txt) for
the [LZ4 block compression
format](https://github.com/lz4/lz4/blob/dev/doc/lz4_Block_format.md).

It's about 500 lines of C code.


## Metadata

Quoting from that LZ4 documentation link's "Metadata" section:

> An LZ4-compressed Block requires additional metadata for proper decoding.
> Typically, a decoder will require the compressed block's size, and an upper
> bound of decompressed size.

TODO: have this library also implement the [LZ4 frame
format](https://github.com/lz4/lz4/blob/dev/doc/lz4_Frame_format.md).


## License

Apache 2. See the [LICENSE](LICENSE) file for details.


---

Updated on November 2022.
