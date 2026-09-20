import { fileURLToPath } from "node:url";
import svgToIco from "svg-to-ico";

// Resolve assets relative to this script so it also works outside the repository root.
const input = fileURLToPath(new URL("../assets/icon.svg", import.meta.url));
const output = fileURLToPath(new URL("../assets/icon.ico", import.meta.url));

// Embed native tray/window sizes as well as the Explorer icon; every image is PNG-compressed.
const sizes = [16, 20, 24, 32, 40, 48, 64, 256];

try {
  await svgToIco({ input_name: input, output_name: output, sizes });
  console.log(`Generated assets/icon.ico from assets/icon.svg (${sizes.join(", ")} px).`);
} catch (error) {
  console.error(`Icon conversion failed: ${error.message}`);
  process.exitCode = 1;
}
