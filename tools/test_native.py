"""Az ESP-IDF saját cJSON verziójával fordítja a hordozható készülékes modellt."""
import os
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def run(*args):
    subprocess.run(args, cwd=ROOT, check=True)

def main():
    source = Path(os.environ["CJSON_PATH"]) if "CJSON_PATH" in os.environ else Path(os.environ["IDF_PATH"]) / "components/json/cJSON"
    with tempfile.TemporaryDirectory(prefix="schema-native-") as folder:
        flags = ["gcc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-g", "-fsanitize=undefined", "-fno-sanitize-recover=all",
                 "-Icomponents/schema_core/include", "-Icomponents/schema_model/include", f"-I{source}"]
        common = ["components/schema_core/schema_core.c", "components/schema_model/schema_model.c",
                  "components/schema_model/schema_catalog.c", "components/schema_model/schema_journal.c", str(source / "cJSON.c")]
        run("python3", "tools/ota_identity.py", str(Path(folder) / "ota_identity.inc"))
        ota = str(Path(folder) / "ota-test")
        run(*flags, "-Itests/native/stubs", "-Imain", f"-I{folder}", "tests/native/ota_test.c", "-o", ota)
        run(ota)
        cli = str(Path(folder) / "model-cli")
        run(*flags, *common, "tests/native/model_cli.c", "-lm", "-o", cli)
        run(*flags, *common, "tests/native/journal_test.c", "-Wl,--wrap=rename", "-lm", "-o", str(Path(folder) / "journal"))
        run(str(Path(folder) / "journal"))
        run(*flags, *common, "components/schema_model/schema_loxone.c", "tests/native/loxone_test.c", "-lm", "-o", str(Path(folder) / "loxone"))
        run(str(Path(folder) / "loxone"))
        run(*flags, *common, "components/schema_model/schema_lx_cache.c", "tests/native/cache_test.c", "-lm", "-o", str(Path(folder) / "cache"))
        run(str(Path(folder) / "cache"))
        run("node", "tests/native_parity.mjs", cli)
        if "MBEDTLS_PATH" in os.environ:
            tls = Path(os.environ["MBEDTLS_PATH"])
            library = [str(tls / "library" / f"{name}.c") for name in ["aes", "cipher", "cipher_wrap", "gcm", "platform_util", "constant_time"]]
            store = str(Path(folder) / "store-cli")
            run(*flags, "-Itests/native/stubs", "-Imain", f"-I{tls / 'include'}", '-DMBEDTLS_CONFIG_FILE="native_mbedtls_config.h"', '-DSM_STATE_DIRECTORY="state"', *common, *library, "main/app_store.c", "tests/native/platform_stubs.c", "tests/native/store_cli.c", "-pthread", "-lm", "-o", store)
            run("python3", "tests/native_store.py", store)
            run("python3", "tools/embed_web.py", str(Path(folder) / "web_bundle.inc"))
            web = str(Path(folder) / "web-cli")
            run(*flags, "-Itests/native/stubs", "-Imain", f"-I{folder}", f"-I{tls / 'include'}", '-DMBEDTLS_CONFIG_FILE="native_mbedtls_config.h"', '-DSM_STATE_DIRECTORY="state"', *common, *library, "main/app_store.c", "main/app_web.c", "tests/native/platform_stubs.c", "tests/native/web_cli.c", "-pthread", "-lm", "-o", web)
            run("python3", "tests/native_web.py", web)

if __name__ == "__main__":
    main()
