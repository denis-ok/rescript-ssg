#!/usr/bin/env node

import path from 'node:path';
import { spawnSync } from "node:child_process";
import * as url from 'url';

const __dirname = url.fileURLToPath(new URL('.', import.meta.url));

const nodeLoaderPath = path.join(__dirname, "node-loader.mjs");

spawnSync(
  "node",
  [`--experimental-loader=${nodeLoaderPath}`, "--no-warnings", ...process.argv.slice(2)],
  {
    stdio: "inherit",
    shell: true,
  }
);
