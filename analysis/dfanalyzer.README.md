

```bash
export DFANALYZER_DIR=/home/haridev/dfanalyzer
git clone https://github.com/LLNL/dfanalyzer ${DFANALYZER_DIR}

python -m venv ./dfanalyzer-env
source ./dfanalyzer-env/bin/activate

cd ${DFANALYZER_DIR}
pip install -r . notebooks/requirements.txt
cd -
```