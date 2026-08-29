import { spawnSync } from "node:child_process";

const platformPresets = {
  linux: {
    builds: ["linux-mingw-debug", "linux-mingw-release"],
    development: "linux-native-debug",
  },
  win32: {
    builds: ["windows-mingw-debug", "windows-mingw-release"],
    development: "windows-mingw-debug",
  },
};

const presets = platformPresets[process.platform];

if (!presets) {
  console.error(`Unsupported platform: ${process.platform}. Use Linux or Windows.`);
  process.exit(1);
}

function run(program, args) {
  console.log(`> ${program} ${args.join(" ")}`);

  const result = spawnSync(program, args, { stdio: "inherit" });

  if (result.error) {
    console.error(result.error.message);
    process.exit(1);
  }

  if (result.status !== 0) {
    process.exit(result.status ?? 1);
  }
}

function configure(preset) {
  run("cmake", ["--preset", preset]);
}

function build(preset, target) {
  const args = ["--build", "--preset", preset];

  if (target) {
    args.push("--target", target);
  }

  run("cmake", args);
}

const task = process.argv[2];

switch (task) {
  case "build":
    for (const preset of presets.builds) {
      configure(preset);
      build(preset);
    }
    break;
  case "format":
  case "format-check":
    configure(presets.development);
    build(presets.development, task);
    break;
  case "test":
    configure(presets.development);
    build(presets.development);
    run("ctest", ["--preset", presets.development]);
    break;
  default:
    console.error("Usage: node scripts/cmake.mjs <build|format|format-check|test>");
    process.exit(1);
}
