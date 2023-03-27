echo "Start run test"

set -e
pytest -s --html tests/report/report.html --root-logdir tests/report/ --maxfail=5 tests && ret=$?
set +e

echo "Run test finished."
