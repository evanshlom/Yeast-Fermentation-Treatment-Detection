# Training-only environment. Not used at runtime on the STM32 —
# the STM32 just runs plain compiled C (ml_model.c), no Python,
# no ONNX runtime, nothing extra needed on-device.
#
# This Dockerfile lives in ml/, but build and run from the PROJECT
# ROOT (not from inside ml/) so training_csvs/ is reachable too:
#
# Build:
#   docker build -t gas-ml -f ml/Dockerfile ml
#
# Run (from your project root):
#   docker run --rm -v "${PWD}:/data" -w /data gas-ml python ml/train_model.py training_csvs/*.csv

FROM python:3.11-slim

RUN pip install --no-cache-dir pandas scikit-learn m2cgen

WORKDIR /data