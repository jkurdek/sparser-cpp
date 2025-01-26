import os
import pyarrow.parquet as pq


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
    directory = "samples/wikipedia/"
    filenames = ["a.parquet", "b.parquet", "c.parquet", "d.parquet"]
    output_file = "samples/wikipedia.json"

    convert_parquet_to_json_streaming(directory, filenames, output_file)
