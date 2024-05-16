#include <QtTest>
#include "object_tree_model.h"

class TestObjectTreeModel : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase()
    {
        // Set up any necessary test data or resources
    }

    void cleanupTestCase()
    {
        // Clean up any test data or resources
    }

    void testModelCreation()
    {
        QObject root;
        ObjectTreeModel model(&root);
        QVERIFY(model.rowCount(QModelIndex()) == 0);
        QVERIFY(model.columnCount(QModelIndex()) == 2);
    }

    void testIndexFunction()
    {
	QObject root;
	QObject child1(&root);
	QObject child2(&root);
	QObject grandchild1(&child1);
	QObject grandchild2(&child1);
	ObjectTreeModel model(&root);

	QModelIndex rootIndex = QModelIndex();
	QVERIFY(!rootIndex.isValid());

	QModelIndex child1Index = model.index(0, 0, QModelIndex());
	QVERIFY(child1Index.isValid());
	QCOMPARE(child1Index.internalPointer(), &child1);

	QModelIndex child2Index = model.index(1, 0, QModelIndex());
	QVERIFY(child2Index.isValid());
	QCOMPARE(child2Index.internalPointer(), &child2);

	QModelIndex grandchild1Index = model.index(0, 0, child1Index);
	QVERIFY(grandchild1Index.isValid());
	QCOMPARE(grandchild1Index.internalPointer(), &grandchild1);

	QModelIndex grandchild2Index = model.index(1, 0, child1Index);
	QVERIFY(grandchild2Index.isValid());
	QCOMPARE(grandchild2Index.internalPointer(), &grandchild2);
    }

    void testData()
    {
	QObject root;
	root.setObjectName("Root");
	QObject child(&root);
	child.setObjectName("Child");
	QObject grandchild(&child);
	grandchild.setObjectName("Grandchild");
	ObjectTreeModel model(&root);

	QModelIndex childIndex = model.index(0, 1, QModelIndex());
	QCOMPARE(model.data(childIndex, Qt::DisplayRole).toString(), QString("Child"));

	QModelIndex grandchildIndex = model.index(0, 1, childIndex);
	QCOMPARE(model.data(grandchildIndex, Qt::DisplayRole).toString(), QString("Grandchild"));
    }

    void testIndexFromObject()
    {
	QObject root;
	QObject child(&root);
	QObject grandchild(&child);
	ObjectTreeModel model(&root);

	QModelIndex childIndex = model.indexFromObject(&child);
	QVERIFY(childIndex.isValid());
	QCOMPARE(childIndex.internalPointer(), &child);

	QModelIndex grandchildIndex = model.indexFromObject(&grandchild);
	QVERIFY(grandchildIndex.isValid());
	QCOMPARE(grandchildIndex.internalPointer(), &grandchild);
    }

    void testParentFunction()
    {
        QObject root;
        QObject child(&root);
        ObjectTreeModel model(&root);

        QModelIndex childIndex = model.index(0, 0, QModelIndex());
        QModelIndex parentIndex = model.parent(childIndex);
        QVERIFY(!parentIndex.isValid());
    }

    void testRowAndColumnCount()
    {
        QObject root;
        QObject child1(&root);
        QObject child2(&root);
        ObjectTreeModel model(&root);

        QCOMPARE(model.rowCount(QModelIndex()), 2);
        QCOMPARE(model.columnCount(QModelIndex()), 2);
    }

    void testEventFilter()
    {
	QObject root;
	ObjectTreeModel model(&root);

	QObject child;
	QSignalSpy childAddedSpy(&model, &ObjectTreeModel::childAdded);
	child.setParent(&root);
	QCOMPARE(childAddedSpy.count(), 1);

	QObject grandchild;
	QSignalSpy grandchildAddedSpy(&model, &ObjectTreeModel::childAdded);
	grandchild.setParent(&child);
	QCOMPARE(grandchildAddedSpy.count(), 1);

	QSignalSpy childRemovedSpy(&model, &ObjectTreeModel::childRemoved);
	child.setParent(nullptr);
	QCOMPARE(childRemovedSpy.count(), 2);

	QSignalSpy grandchildRemovedSpy(&model, &ObjectTreeModel::childRemoved);
	grandchild.setParent(nullptr);
	QCOMPARE(grandchildRemovedSpy.count(), 0);
    }

};

QTEST_MAIN(TestObjectTreeModel)
#include "test_object_tree_model.moc"
