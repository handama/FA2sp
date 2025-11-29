import sys
import random
import textwrap

def generate_obfuscated_cpp(key):
    if len(key) != 32:
        raise ValueError("Key must be exactly 32 characters (256 bits)")

    key_bytes = [ord(c) for c in key]
    
    var_names = [f"v{i:02x}" for i in range(32)]
    func_names = [f"f{i:02x}" for i in range(8)]
    
    # Create expressions for each key byte
    expressions = []
    for i, byte in enumerate(key_bytes):
        layers = random.randint(3, 6)
        masks = []
        
        total_xor = 0
        for j in range(layers - 1):
            mask = random.randint(1, 255)
            masks.append(mask)
            total_xor ^= mask
        
        final_mask = byte ^ total_xor
        masks.append(final_mask)
        
        expr_parts = [str(m) for m in masks]
        random.shuffle(expr_parts)
        expr = " ^ ".join(expr_parts)
        expressions.append(f"static_cast<unsigned char>({expr})")
    
    # Randomize the assignment of key bytes to functions
    byte_indices = list(range(32))
    random.shuffle(byte_indices)
    
    # Create function definitions with randomized key byte assignments
    func_definitions = []
    for i in range(8):
        func_body = []
        func_body.append(f"void {func_names[i]}(unsigned char* part) {{")
        
        for j in range(4):
            idx = byte_indices[i * 4 + j]  # Use randomized byte index
            func_body.append(f"    unsigned char {var_names[idx]} = {expressions[idx]};")
            func_body.append(f"    part[{j}] = {var_names[idx]};")
        
        func_body.append("}")
        func_definitions.append("\n".join(func_body))
    
    # Create key assembly with corresponding randomized indices
    key_assembly = []
    key_assembly.append("    unsigned char part[4];")
    for i in range(8):
        func_name = f"f{i:02x}"
        # Map function to the original key byte positions
        offset = byte_indices[i * 4:i * 4 + 4]
        key_assembly.append(f"    EncryptionKey::{func_name}(part);")
        for j, byte_idx in enumerate(offset):
            key_assembly.append(f"    key[{byte_idx}] = part[{j}];")
    
    # Template for the complete C++ file
    cpp_template = """#include "Body.h"
#pragma optimize("", off)

namespace EncryptionKey {
%(func_definitions)s
}

std::array<uint8_t, 32> ResourcePack::get_aes_key() {
    std::array<uint8_t, 32> key = {};
%(randomized_key_assembly)s
    return key;
}
#pragma optimize("", on)
"""
    return cpp_template % {
        'func_definitions': "\n\n".join(func_definitions),
        'randomized_key_assembly': "\n".join(key_assembly)
    }

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python key_obf.py <32_character_key>")
        sys.exit(1)
    
    key = sys.argv[1]
    try:
        cpp_code = generate_obfuscated_cpp(key)
        with open("Body.Encryption.cpp", "w") as f:
            f.write(cpp_code)
        print("C++ code has been written to Body.Encryption.cpp")
    except ValueError as e:
        print(e)
        sys.exit(1)