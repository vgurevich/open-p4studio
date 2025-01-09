all: clean build install

clean:
	@echo "Cleaning..."
	rm -rf build dist bf_pktpy.egg-info
	@echo "Cleaning... Done"

build:
	@echo "Building..."
	python3 setup.py sdist bdist_wheel --universal
	@echo "Building... Done"

format:
	@echo "Formatting..."
	python -m black .
	@echo "Formatting... Done"

check:
	python -m black --check .

install:
	@echo "Installing..."
	python3 -m pip install dist/bf_pktpy-*.whl --force
	@echo "Installing... Done"
