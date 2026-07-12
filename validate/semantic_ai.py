#!/usr/bin/env python3
"""Simple semantic AI layer for interpreting house-load data using embeddings.

This uses SentenceTransformer to create embeddings for text descriptions and
then performs similarity search over a small knowledge base.
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import List, Dict, Tuple

from sentence_transformers import SentenceTransformer
import numpy as np


class SemanticAI:
    def __init__(self, model_name: str = "all-MiniLM-L6-v2") -> None:
        self.model = SentenceTransformer(model_name)

    def embed(self, texts: List[str]) -> np.ndarray:
        return self.model.encode(texts, convert_to_numpy=True)

    def find_best_matches(self, query: str, knowledge_base: List[str], top_k: int = 3) -> List[Tuple[str, float]]:
        if not knowledge_base:
            return []
        query_embedding = self.model.encode([query], convert_to_numpy=True)[0]
        kb_embeddings = self.model.encode(knowledge_base, convert_to_numpy=True)
        scores = np.dot(kb_embeddings, query_embedding)
        ranked = sorted(enumerate(scores), key=lambda item: item[1], reverse=True)
        return [(knowledge_base[idx], float(score)) for idx, score in ranked[:top_k]]


def load_text_documents(base_dir: Path) -> List[str]:
    documents: List[str] = []
    for path in [base_dir / "house_tree.json", base_dir / "csv" / "expert_system_report.txt"]:
        if path.exists():
            documents.append(path.read_text(encoding="utf-8")[:4000])
    return documents


def build_knowledge_base() -> List[str]:
    base_dir = Path(__file__).resolve().parent
    tree_path = base_dir / "house_tree.json"
    report_path = base_dir / "csv" / "expert_system_report.txt"
    docs: List[str] = []
    if tree_path.exists():
        docs.append(f"House tree data: {tree_path.read_text(encoding='utf-8')[:4000]}")
    if report_path.exists():
        docs.append(f"Expert system report: {report_path.read_text(encoding='utf-8')[:4000]}")
    return docs


def main() -> None:
    parser = argparse.ArgumentParser(description="Use semantic embeddings to interpret house-load data")
    parser.add_argument("--query", default="What is the main risk in this house load system?", help="Question or instruction for the semantic AI")
    parser.add_argument("--top-k", type=int, default=3, help="Number of matches to return")
    args = parser.parse_args()

    ai = SemanticAI()
    knowledge_base = build_knowledge_base()
    matches = ai.find_best_matches(args.query, knowledge_base, top_k=args.top_k)
    print(f"Query: {args.query}")
    print("Matches:")
    for text, score in matches:
        preview = " ".join(text.split())
        print(f"- score={score:.3f}: {preview[:700]}")


if __name__ == "__main__":
    main()
