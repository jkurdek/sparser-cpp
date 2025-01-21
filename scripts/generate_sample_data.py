import json
import random
from typing import List, Dict, Any


def generate_json_records(
    num_records: int, schema: Dict[str, Any], key: str, value: Any, percentage: float
) -> List[str]:
    """
    Generate newline-delimited JSON records.

    :param num_records: Number of JSON records to generate.
    :param schema: A dictionary defining the schema for JSON records.
    :param key: The key for which a specific value will be used.
    :param value: The value to use for the specified key.
    :param percentage: Percentage of records that will have the specified value for the key.
    :return: A list of JSON strings (one per line).
    """
    records = []
    for _ in range(num_records):
        record = {}
        for field, field_type in schema.items():
            if field == key and random.random() < percentage / 100:
                record[field] = value
            else:
                record[field] = generate_value(field_type)
        records.append(json.dumps(record))
    return records


def generate_value(field_type: Any) -> Any:
    """
    Generate a random value based on the field type.

    :param field_type: Type of the field, can be str, int, float, list, or dict.
    :return: A randomly generated value.
    """
    if field_type == str:
        return random.choice(["Lord", "beta", "gamma", "Rings"])
    elif field_type == int:
        return random.randint(0, 100)
    elif field_type == float:
        return round(random.uniform(0, 100), 2)
    elif field_type == list:
        return random.sample(["a", "b", "c", "d", "e"], k=random.randint(1, 3))
    elif field_type == dict:
        return {
            "subfield1": random.choice(["x", "y", "z"]),
            "subfield2": random.randint(1, 10),
        }
    else:
        return None


if __name__ == "__main__":
    schema_definition = {
        "id": int,
        "text": str,
        "score": float,
        "tags": list,
        "details": dict,
    }

    json_records = generate_json_records(
        num_records=100000,
        schema=schema_definition,
        key="text",
        value="Lord of the Rings is my favorite movie. I have watched it 100 times.",
        percentage=30,
    )

    with open("samples/generated_records.json", "w") as file:
        file.write("\n".join(json_records))

    print("JSON records have been written to 'samples/generated_records.json'")
