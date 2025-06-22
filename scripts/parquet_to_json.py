import os
import argparse
import pyarrow.parquet as pq
import requests
import zipfile


def download_file(url, output_path):
    """Download a file from a URL to a specified path."""
    print(f"Downloading dataset from {url} to {output_path}...")
    response = requests.get(url, stream=True)
    response.raise_for_status()
    with open(output_path, "wb") as f:
        for chunk in response.iter_content(chunk_size=8192):
            f.write(chunk)
    print("Download complete.")


def extract_zip(zip_path, extract_to):
    """Extract a ZIP file to a specified directory."""
    print(f"Extracting {zip_path} to {extract_to}...")
    with zipfile.ZipFile(zip_path, "r") as zip_ref:
        zip_ref.extractall(extract_to)
    print("Extraction complete.")


def convert_parquet_to_json_streaming(
    directory: str, filenames: list, output_file: str, batch_size: int = 10_000
) -> None:
    """
    Convert a list of Parquet files to a single JSON Lines file in a memory-efficient manner.
    Data is read and written in batches to avoid loading entire datasets into memory at once.

    :param directory: Directory containing the Parquet files.
    :param filenames: List of Parquet files to be combined.
    :param output_file: Output JSON file (JSON Lines format).
    :param batch_size: Number of rows per batch (adjust as needed).
    """
    with open(output_file, "w", encoding="utf-8") as f_out:
        for filename in filenames:
            filepath = os.path.join(directory, filename)
            print(f"Started processing: {filepath}")

            parquet_file = pq.ParquetFile(filepath)

            for batch in parquet_file.iter_batches(batch_size=batch_size):
                df = batch.to_pandas()
                json_str = df.to_json(orient="records", lines=True)
                f_out.write(json_str)

    print(f"Conversion to JSON completed successfully! Output saved to: {output_file}")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="Download, extract, and convert Parquet files to JSON Lines format."
    )
    parser.add_argument(
        "--download_url", required=True, help="URL to download the dataset."
    )
    parser.add_argument(
        "--download_dir", required=True, help="Directory to save the downloaded file."
    )
    parser.add_argument(
        "--extract_dir", required=True, help="Directory to extract the dataset."
    )
    parser.add_argument(
        "--output_file",
        required=True,
        help="Output JSON file (JSON Lines format).",
    )
    parser.add_argument(
        "--include_files",
        default="",
        help="Comma-separated list of Parquet files to include in the JSON output.",
    )
    args = parser.parse_args()

    # Step 1: Download the dataset
    zip_file_path = os.path.join(args.download_dir, "dataset.zip")
    # download_file(args.download_url, zip_file_path)

    # # Step 2: Extract the dataset
    # extract_zip(zip_file_path, args.extract_dir)

    # Step 3: Convert Parquet files to JSON
    include_files = args.include_files.split(",") if args.include_files else None
    if include_files:
        filenames = [f for f in os.listdir(args.extract_dir) if f in include_files]
    else:
        filenames = [f for f in os.listdir(args.extract_dir) if f.endswith(".parquet")]

    convert_parquet_to_json_streaming(args.extract_dir, filenames, args.output_file)
