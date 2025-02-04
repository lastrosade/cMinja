# cMinja

A limited and unsafe C++ Jinja Templater.

Uses [minja.hpp](https://github.com/google/minja), [nlohmann::json](https://github.com/nlohmann/json) and [lightweight-yaml-parser](https://github.com/MaxAve/lightweight-yaml-parser)

Reads either yaml or json, formats it according to a minja template, returns the result.

## Build

cmake .. -G Ninja && ninja

## Usage

```
Usage: cminja <flags>
    -h prints help
    -v prints versions
    -j json
    -y yaml
    -i path to minja file
    -d path to data
    -s read data from stdin
    -o save to file
```

```
cminja -jd ..\test\simple.json -i ..\test\chatml.m2
```
