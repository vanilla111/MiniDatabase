import random

createTableSQL = "create table test(id int, %s, primary key(id));"

cols = []
for i in range(1, 100):
    cols.append("col_%s int" % i)

createTableSQL = createTableSQL % ",".join(cols)

insertSQL = "insert into test values(%s);"
insertSQLArr = []
for i in range(0, 200000):
    values = []
    values.append(str(i))
    for j in range(1, 100):
        values.append(str(random.randint(0, 100000)))
    insertSQLArr.append(insertSQL % ",".join(values))

with open("test.sql", 'w') as f:
    f.write(createTableSQL + "\n");
    for insert in insertSQLArr:
        f.write(insert + "\n");
