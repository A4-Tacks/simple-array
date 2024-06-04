A data structure language composed of string values and their accompanying value,
The value itself is linked to the next associated value through a chain structure

This design makes its use more focused on "what needs to be handled" rather than "what exists"

The grammar and implementation are very simple, which can meet general requirements,
and there is no hash table dependency

It works in ASCII, but there won't be any issues with UTF-8 input as it mostly uses exclusion sets

By returning the size to be allocated during the parse process,
it is possible to quickly customize the allocation using methods such as `alloca()`

Example
---

```
list {
    button {
        name { foo }
        pos { 10 20 }
    }
    button {
        name { bar }
        pos { 10 25 }
    }
    list {
        name { list }
        pos { 20 20 }
        members {
            button {
                name { bar }
                pos { 0 0 }
            }
        }
    }
}
```

like JSON, but array is linked list

```json
["list", [
    ["button", [
        ["name", ["foo"]],
        ["pos", [["10"], ["20"]]]
    ]],
    ["button", [
        ["name", ["bar"]],
        ["pos", [["10"], ["25"]]]
    ]],
    ["list", [
        ["name", "list"],
        ["pos", [["20"], ["20"]]],
        ["members", [
            ["button", [
                ["name", ["bar"]],
                ["pos", [["0"], ["0"]]]
            ]]
        ]]
    ]]
]]
```

# Grammar

**sipa** = ws \*(value ws)

**value** = string [ array ] / array

**array** = begin ws \*(value ws) end

**newline** = `\r?\n`

**ws** = `[ \t]*` \*([ comment ] newline)

**begin** = "{"

**end** = "}"

**string** = `[^"'{}; \t\r\n]+` / `'[^'\r\n]*'` / `"` str-ch* `"`

**str-ch** = `[^"\r\n\\]`\
**str-ch**/= `\\` (`[nrtae"\\]` / `x(?_*[\da-fA-F]){2}` / `x\{(?:(?:_*[\da-fA-F]{2}))+\}` / [ comment ] `\r?\n[ \t]*`)

**comment** = `;[^\r\n]*`
