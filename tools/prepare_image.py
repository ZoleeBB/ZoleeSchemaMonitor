"""Munkaképből kis, metaadatmentes PNG/JPEG. Nem vág, nem változtatja a portok helyét."""
import argparse
from io import BytesIO
from pathlib import Path
import warnings

from image_policy import PROFILES


def prepare(source, role, image_format="PNG"):
    from PIL import Image, ImageOps

    with warnings.catch_warnings():
        warnings.simplefilter("error", Image.DecompressionBombWarning)
        with Image.open(source) as original:
            if getattr(original, "is_animated", False):
                raise ValueError("Animált forrás nem használható statikus elemképként")
            image = ImageOps.exif_transpose(original).convert("RGBA")
    profile = PROFILES[role]
    extension = {"PNG": ".png", "JPEG": ".jpg"}.get(image_format)
    if extension not in profile["extensions"]:
        raise ValueError("Ehhez a képszerephez nem engedélyezett a kimeneti formátum")
    target = (profile["width"], profile["height"])
    if role == "element":
        # A mozgó réteg és a csatlakozók a teljes vászonhoz regisztráltak.
        # Automatikus körbevágás vagy nyújtás ezt eltolná.
        if image.width != image.height or image.width < target[0]:
            raise ValueError("Legalább 160×160-as négyzetes forrás szükséges; felméretezés nincs")
        image = image.resize(target, Image.Resampling.LANCZOS)
    else:
        image.thumbnail(target, Image.Resampling.LANCZOS)
    clean = Image.new("RGBA", image.size)
    clean.paste(image)
    output = BytesIO()
    if image_format == "JPEG":
        if clean.getchannel("A").getextrema() != (255, 255):
            raise ValueError("JPEG csak teljesen fedő képből készíthető")
        clean.convert("RGB").save(output, format="JPEG", quality=90, optimize=True, subsampling=0)
    elif image_format == "PNG":
        clean.save(output, format="PNG", optimize=True, compress_level=9)
    else:
        raise ValueError("Nem támogatott kimeneti formátum")
    data = output.getvalue()
    if len(data) > profile["bytes"]:
        raise ValueError("Az átméretezett kép túl nagy; egyszerűsítsd a képet")
    return data, image.size


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--role", choices=PROFILES, required=True)
    args = parser.parse_args()
    if args.source.resolve() == args.output.resolve():
        parser.error("A munkaképet nem írjuk felül")
    if args.output.suffix not in {".png", ".jpg"}:
        parser.error("A kimenet PNG vagy dokumentációs JPEG legyen")
    try:
        data, size = prepare(args.source, args.role, "JPEG" if args.output.suffix == ".jpg" else "PNG")
    except (ValueError, OSError, ImportError) as error:
        parser.exit(1, f"Képelőkészítés sikertelen: {error}\n")
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(data)
    print(f"{args.output}: {size[0]}×{size[1]}, {len(data)} bájt")


if __name__ == "__main__":
    main()
