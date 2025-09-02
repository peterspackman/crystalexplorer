#include <catch2/catch_test_macros.hpp>
#include <QObject>
#include <QModelIndex>
#include "object_tree_model.h"

TEST_CASE("ObjectTreeModel creation", "[model][tree]") {
    QObject root;
    ObjectTreeModel model(&root);
    
    REQUIRE(model.rowCount(QModelIndex()) == 0);
    REQUIRE(model.columnCount(QModelIndex()) == 2);
}

TEST_CASE("ObjectTreeModel index function", "[model][tree]") {
    QObject root;
    QObject child1(&root);
    QObject child2(&root);
    QObject grandchild1(&child1);
    QObject grandchild2(&child1);
    ObjectTreeModel model(&root);

    SECTION("Root index") {
        QModelIndex rootIndex = QModelIndex();
        REQUIRE_FALSE(rootIndex.isValid());
    }

    SECTION("Child indices") {
        QModelIndex child1Index = model.index(0, 0, QModelIndex());
        REQUIRE(child1Index.isValid());
        REQUIRE(child1Index.internalPointer() == &child1);

        QModelIndex child2Index = model.index(1, 0, QModelIndex());
        REQUIRE(child2Index.isValid());
        REQUIRE(child2Index.internalPointer() == &child2);
    }

    SECTION("Grandchild indices") {
        QModelIndex child1Index = model.index(0, 0, QModelIndex());
        
        QModelIndex grandchild1Index = model.index(0, 0, child1Index);
        REQUIRE(grandchild1Index.isValid());
        REQUIRE(grandchild1Index.internalPointer() == &grandchild1);

        QModelIndex grandchild2Index = model.index(1, 0, child1Index);
        REQUIRE(grandchild2Index.isValid());
        REQUIRE(grandchild2Index.internalPointer() == &grandchild2);
    }
}

TEST_CASE("ObjectTreeModel data retrieval", "[model][tree]") {
    QObject root;
    root.setObjectName("Root");
    QObject child(&root);
    child.setObjectName("Child");
    QObject grandchild(&child);
    grandchild.setObjectName("Grandchild");
    ObjectTreeModel model(&root);

    SECTION("Child data") {
        QModelIndex childIndex = model.index(0, 1, QModelIndex());
        REQUIRE(model.data(childIndex, Qt::DisplayRole).toString() == QString("Child"));
    }

    SECTION("Grandchild data") {
        QModelIndex childIndex = model.index(0, 1, QModelIndex());
        QModelIndex grandchildIndex = model.index(0, 1, childIndex);
        REQUIRE(model.data(grandchildIndex, Qt::DisplayRole).toString() == QString("Grandchild"));
    }
}

TEST_CASE("ObjectTreeModel indexFromObject", "[model][tree]") {
    QObject root;
    QObject child(&root);
    QObject grandchild(&child);
    ObjectTreeModel model(&root);

    SECTION("Child index from object") {
        QModelIndex childIndex = model.indexFromObject(&child);
        REQUIRE(childIndex.isValid());
        REQUIRE(childIndex.internalPointer() == &child);
    }

    SECTION("Grandchild index from object") {
        QModelIndex grandchildIndex = model.indexFromObject(&grandchild);
        REQUIRE(grandchildIndex.isValid());
        REQUIRE(grandchildIndex.internalPointer() == &grandchild);
    }
}

TEST_CASE("ObjectTreeModel parent function", "[model][tree]") {
    QObject root;
    QObject child(&root);
    ObjectTreeModel model(&root);

    QModelIndex childIndex = model.index(0, 0, QModelIndex());
    QModelIndex parentIndex = model.parent(childIndex);
    
    REQUIRE_FALSE(parentIndex.isValid());
}

TEST_CASE("ObjectTreeModel row and column count", "[model][tree]") {
    QObject root;
    QObject child1(&root);
    QObject child2(&root);
    ObjectTreeModel model(&root);

    REQUIRE(model.rowCount(QModelIndex()) == 2);
    REQUIRE(model.columnCount(QModelIndex()) == 2);
}

