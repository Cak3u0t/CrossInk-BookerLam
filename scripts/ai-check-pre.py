import os
import subprocess
Import("env")

def run_ai_check(source, target, env):
    script_path = os.path.join(env["PROJECT_DIR"], "scripts", "ai-check.sh")
    if os.path.exists(script_path):
        print("🔍 Running AI safety checks...")
        result = subprocess.run(["bash", script_path], cwd=env["PROJECT_DIR"])
        if result.returncode != 0:
            print("❌ AI checks failed. Aborting build.")
            exit(1)
    else:
        print("⚠️  ai-check.sh not found. Skipping validation.")

env.AddPreAction("buildprog", run_ai_check)
