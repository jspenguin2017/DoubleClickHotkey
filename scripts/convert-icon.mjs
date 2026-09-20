import { fileURLToPath } from "node:url";
import svgToIco from "svg-to-ico";

// Resolve assets relative to this script so it also works outside the repository root.
const input = fileURLToPath(new URL("../assets/icon.svg", import.meta.url));
const output = fileURLToPath(new URL("../assets/icon.ico", import.meta.url));

// svg-to-ico embeds PNG-compressed images; keep only the largest size.
const sizes = [256];

try {
  await svgToIco({ input_name: input, output_name: output, sizes });
  console.log(`Generated assets/icon.ico from assets/icon.svg (${sizes.join(", ")} px).`);
} catch (error) {
  console.error(`Icon conversion failed: ${error.message}`);
  process.exitCode = 1;
}
