"""
엔진 도감(Assets/Guide/EngineGuide.json)이 지금 코드와 git 기록에 맞는지 검사한다.

    python Tools/Guide/check_guide.py

카드가 없는 클래스, 코드에서 사라진 함수 이름, 어느 단계에도 적지 않은 커밋이 있으면 알려 주고 1로 끝난다.
아직 적지 않은 커밋은 도감 파일에 그대로 붙여 넣을 수 있게 출력한다. 파일은 읽기만 한다.

커밋은 자기 번호를 미리 알 수 없다. 그래서 도감과 이 도구만 고친 마지막 커밋들은 적지 않아도 된다.
"""

import json
import os
import re
import subprocess
import sys

PROJECT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", ".."))
GUIDE = os.path.join(PROJECT, "Assets", "Guide", "EngineGuide.json")
SOURCE_EXTENSIONS = (".cpp", ".h", ".hlsl", ".cs")
# 이 폴더만 고친 커밋은 도감을 정리한 커밋이다.
GUIDE_FOLDERS = ("Assets/Guide/", "Tools/Guide/")

problems = []


def check(condition, message):
    if not condition:
        problems.append(message)


def git(*arguments):
    command = ["git", "-c", "core.quotepath=false", "-C", PROJECT, *arguments]
    return subprocess.run(command, capture_output=True, encoding="utf-8", check=True).stdout


def stem(path):
    return os.path.splitext(path)[0]


def read_source(card):
    """카드에 적힌 파일과, 이름이 같고 확장자만 다른 파일을 모두 읽는다."""
    text = ""
    for extension in SOURCE_EXTENSIONS:
        path = os.path.join(PROJECT, stem(card["file"]) + extension)
        if os.path.exists(path):
            with open(path, encoding="utf-8-sig") as file:
                text += file.read()
    return text


def check_links(guide):
    """카드, 기능, 단계, 묶음이 실제 있는 카드를 가리키는지 본다."""
    ids = [item["id"] for kind in ("cards", "features", "stages") for item in guide[kind]]
    for duplicate in sorted({item for item in ids if ids.count(item) > 1}):
        check(False, f"이름이 겹칩니다: {duplicate}")

    cards = {card["id"] for card in guide["cards"]}
    grouped = [card for group in guide["groups"] for card in group["cards"]]
    for card in sorted(cards):
        check(grouped.count(card) == 1, f"카드 {card}가 묶음에 {grouped.count(card)}번 들어 있습니다.")

    references = [(group["name"], card) for group in guide["groups"] for card in group["cards"]]
    references += [(card["id"], link) for card in guide["cards"] for link in card["links"]]
    references += [(feature["id"], step["card"]) for feature in guide["features"] for step in feature["steps"]]
    references += [(stage["id"], card) for stage in guide["stages"] for commit in stage["commits"] for card in commit["cards"]]
    for owner, card in references:
        check(card in cards, f"{owner}: 없는 카드 {card}를 가리킵니다.")


def check_functions(guide):
    """카드와 기능에 적은 함수 이름이 그 카드의 파일에 있는지 본다."""
    for card in guide["cards"]:
        check(os.path.exists(os.path.join(PROJECT, card["file"])), f"카드 {card['id']}의 파일 {card['file']}이 없습니다.")
        source = read_source(card)
        texts = [function["name"] for function in card["functions"]]
        texts += [step["function"] for feature in guide["features"] for step in feature["steps"] if step["card"] == card["id"]]
        for name in (part for text in texts for part in text.split(" · ")):
            name = re.sub(r"\s*\(.*\)$", "", name)
            # 우리말로 적은 것은 함수 이름이 아니라 코드의 한 부분을 설명한 것이다.
            if re.search("[가-힣]", name):
                continue
            check(re.search(r"\b" + re.escape(name) + r"\b", source), f"{card['file']}에 {name}이 없습니다(카드 {card['id']}).")


def check_classes(guide):
    """모든 클래스와 헤더에 카드가 있는지 보고, 클래스 개수를 돌려준다."""
    names = {card["name"] for card in guide["cards"]}
    stems = {stem(card["file"]) for card in guide["cards"]}
    count = 0
    for file_name in sorted(os.listdir(PROJECT)):
        if not file_name.endswith(".h"):
            continue
        with open(os.path.join(PROJECT, file_name), encoding="utf-8-sig") as file:
            classes = re.findall(r"^class (\w+) final", file.read(), re.M)
        count += len(classes)
        for name in classes:
            check(name in names, f"클래스 {name}의 카드가 없습니다({file_name}).")
        check(stem(file_name) in stems or set(classes) & names, f"{file_name}의 카드가 없습니다.")
    return count


def read_commits():
    """오래된 것부터 (번호, 날짜, 제목, 고친 파일, 처음 더한 파일)을 읽는다."""
    commits = []
    text = git("log", "--reverse", "--no-renames", "--name-status", "--date=format:%Y.%m.%d", "--format=@%H%x09%ad%x09%s")
    for line in text.splitlines():
        if line.startswith("@"):
            number, date, title = line[1:].split("\t", 2)
            commits.append({"hash": number, "date": date, "title": title, "files": [], "added": []})
        elif line:
            status, path = line.split("\t", 1)
            commits[-1]["files"].append(path)
            if status == "A":
                commits[-1]["added"].append(path)
    return commits


def find_new_cards(guide, commits):
    """커밋마다, 그 커밋에서 파일이 처음 생긴 카드를 찾는다."""
    first = {}
    for commit in commits:
        for path in commit["added"]:
            first.setdefault(stem(path), commit["hash"])
    new_cards = {commit["hash"]: [] for commit in commits}
    for card in guide["cards"]:
        number = first.get(stem(card["file"]))
        if number:
            new_cards[number].append(card["id"])
        else:
            print(f"아직 커밋하지 않은 카드: {card['id']} ({card['file']})")
    return new_cards


def check_stages(guide, commits):
    """단계에 적은 커밋이 git과 같은지 보고, 아직 적지 않은 커밋을 돌려준다."""
    new_cards = find_new_cards(guide, commits)
    listed = [(stage, commit) for stage in guide["stages"] for commit in stage["commits"]]

    for (stage, commit), real in zip(listed, commits):
        place = f"{stage['id']}의 {commit['hash']}"
        if not real["hash"].startswith(commit["hash"]):
            check(False, f"{place} 자리에 올 커밋: {real['hash'][:7]} {real['title']}")
            continue
        check(commit["title"] == real["title"], f"{place}의 제목이 git과 다릅니다: {real['title']}")
        check(sorted(commit["cards"]) == sorted(new_cards[real["hash"]]), f"{place}에서 생긴 카드: {new_cards[real['hash']]}")
    check(len(listed) <= len(commits), "단계에 적은 커밋이 git보다 많습니다.")

    dates = {commit["hash"]: commit["date"] for commit in commits}
    for stage in guide["stages"]:
        days = [date for commit in stage["commits"] for number, date in dates.items() if number.startswith(commit["hash"])]
        if days:
            expected = days[0] if days[0] == days[-1] else f"{days[0]} – {days[-1][5:]}"
            check(stage["date"] == expected, f"{stage['id']}의 날짜가 git과 다릅니다: {expected}")

    missing = [(commit, new_cards[commit["hash"]]) for commit in commits[len(listed):]]
    while missing and all(path.startswith(GUIDE_FOLDERS) for path in missing[-1][0]["files"]):
        missing.pop()
    return missing


def main():
    sys.stdout.reconfigure(encoding="utf-8")
    with open(GUIDE, encoding="utf-8") as file:
        guide = json.load(file)

    check_links(guide)
    check_functions(guide)
    class_count = check_classes(guide)
    missing = check_stages(guide, read_commits())
    for commit, cards in missing:
        line = {"hash": commit["hash"][:7], "title": commit["title"], "cards": cards}
        check(False, "아직 적지 않은 커밋: " + json.dumps(line, ensure_ascii=False))

    for problem in problems:
        print(problem)
    commit_count = sum(len(stage["commits"]) for stage in guide["stages"])
    print(f"카드 {len(guide['cards'])}장, 클래스 {class_count}개, 기능 {len(guide['features'])}개, "
          f"단계 {len(guide['stages'])}개, 커밋 {commit_count}개: 문제 {len(problems)}개")
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
